#include "PlaybackEngine.h"
#include "ffmpeg/FFmpegContext.h"
#include "ffmpeg/ContainerReader.h"
#include "decoder/PacketQueue.h"
#include "decoder/VideoDecoder.h"
#include "decoder/AudioDecoder.h"
#include "decoder/SubtitleDecoder.h"
#include "video/FrameQueue.h"
#include "video/VideoClock.h"
#include "audio/AudioQueue.h"
#include "audio/AudioClock.h"
#include "audio/AudioOutput.h"
#include "subtitles/SubtitleQueue.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/display.h>
}

#include <cmath>
#include <cstring>

namespace rav {

PlaybackEngine::PlaybackEngine() {
    audio_output_ = std::make_unique<AudioOutput>();
    audio_output_->set_fill_callback([this](uint8_t* data, int frames) {
        return fill_audio_buffer(data, frames);
    });
}

PlaybackEngine::~PlaybackEngine() { close(); }

bool PlaybackEngine::open(const std::string& url) {
    if (state_.load() != PlayerState::Idle &&
        state_.load() != PlayerState::Stopped) {
        return false;
    }

    close();

    set_state(PlayerState::Loading);

    if (!reader_.open(url)) {
        set_state(PlayerState::Error);
        return false;
    }

    const auto& info = reader_.info();
    duration_ = info.duration > 0
        ? static_cast<double>(info.duration) / AV_TIME_BASE : 0.0;

    // Extract metadata
    auto* fmt_ctx = reader_.context();
    auto* dict = fmt_ctx->metadata;
    auto read_tag = [&](const char* key) -> std::string {
        auto* entry = av_dict_get(dict, key, nullptr, 0);
        return entry ? entry->value : "";
    };
    media_title_ = read_tag("title");
    media_artist_ = read_tag("artist");
    media_album_ = read_tag("album");
    media_bitrate_ = static_cast<int>(info.bit_rate);

    video_stream_ = -1;
    audio_stream_ = -1;
    subtitle_stream_ = -1;

    for (const auto& s : info.streams) {
        if (s.type == AVMEDIA_TYPE_VIDEO && video_stream_ < 0)
            video_stream_ = s.index;
        if (s.type == AVMEDIA_TYPE_AUDIO && audio_stream_ < 0)
            audio_stream_ = s.index;
        if (s.type == AVMEDIA_TYPE_SUBTITLE && subtitle_stream_ < 0)
            subtitle_stream_ = s.index;
    }

    if (video_stream_ >= 0) {
        auto* stream = reader_.context()->streams[video_stream_];
        if (!video_decoder_.open(stream->codecpar)) {
            video_stream_ = -1;
        } else {
            video_rotation_ = get_video_rotation();
        }
    }

    if (audio_stream_ >= 0) {
        auto* stream = reader_.context()->streams[audio_stream_];
        if (!audio_decoder_.open(stream->codecpar)) {
            audio_stream_ = -1;
        }
    }

    if (subtitle_stream_ >= 0) {
        auto* stream = reader_.context()->streams[subtitle_stream_];
        if (!subtitle_decoder_.open(stream->codecpar)) {
            subtitle_stream_ = -1;
        }
    }

    if (video_stream_ < 0 && audio_stream_ < 0) {
        set_state(PlayerState::Error);
        return false;
    }

    event_bus_.publish(MediaLoadedEvent{duration_});
    set_state(PlayerState::Stopped);
    return true;
}

void PlaybackEngine::close() {
    stop();
    reader_.close();
    video_decoder_.close();
    audio_decoder_.close();
    subtitle_decoder_.close();
    video_queue_.reset();
    audio_queue_.reset();
    subtitle_queue_.clear();
    leftover_audio_frames_ = 0;
    leftover_audio_offset_ = 0;
    current_audio_frame_ = AudioFrame();
    video_clock_.reset();
    audio_clock_.reset();
    duration_ = 0.0;
    video_stream_ = -1;
    audio_stream_ = -1;
    subtitle_stream_ = -1;
    if (swr_ctx_) {
        swr_free(&swr_ctx_);
        swr_ctx_ = nullptr;
    }
    av_channel_layout_uninit(&last_in_ch_layout_);
    std::memset(&last_in_ch_layout_, 0, sizeof(last_in_ch_layout_));
    last_in_sample_fmt_ = AV_SAMPLE_FMT_NONE;
    last_in_sample_rate_ = 0;
    video_width_ = 0;
    video_height_ = 0;
    video_rotation_ = 0;
    set_state(PlayerState::Idle);
}

void PlaybackEngine::play() {
    std::lock_guard<std::mutex> lock(control_mutex_);
    if (state_.load() != PlayerState::Stopped &&
        state_.load() != PlayerState::Paused) {
        return;
    }

    if (state_.load() == PlayerState::Paused) {
        resume_internal();
        return;
    }

    flush_buffers();
    audio_clock_.reset();
    audio_clock_.start();
    video_clock_.reset();
    video_clock_.start();
    audio_started_ = false;
    video_clock_locked_ = false;

    if (audio_output_ && audio_stream_ >= 0) {
        AudioOutputSpec spec;
        spec.sample_rate = audio_decoder_.sample_rate();
        spec.format = AV_SAMPLE_FMT_FLT; // Force float packed format
        auto* stream = reader_.context()->streams[audio_stream_];
        auto* par = stream->codecpar;
        spec.channels = 2; // Force stereo downmix
        spec.channel_layout = AV_CH_LAYOUT_STEREO;
        bool ok = audio_output_->init(spec);
        fprintf(stderr, "play: audio_init(sr=%d ch=%d) -> %d\n",
                spec.sample_rate, spec.channels, ok);

        int bytes_per_sample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
        if (bytes_per_sample <= 0) bytes_per_sample = 4;
        audio_clock_.set_bytes_per_second(
            spec.sample_rate * spec.channels * bytes_per_sample);
    }

    set_state(PlayerState::Playing);
    running_ = true;
    flush_requested_ = false;

    // Join any existing threads
    if (playback_thread_.joinable()) playback_thread_.join();
    if (video_decode_thread_.joinable()) video_decode_thread_.join();
    if (audio_decode_thread_.joinable()) audio_decode_thread_.join();

    // Start demuxer thread and decode threads
    playback_thread_ = std::thread(&PlaybackEngine::demux_thread_fn, this);
    if (video_stream_ >= 0) {
        video_decode_thread_ = std::thread(&PlaybackEngine::video_decode_thread_fn, this);
    }
    if (audio_stream_ >= 0) {
        audio_decode_thread_ = std::thread(&PlaybackEngine::audio_decode_thread_fn, this);
    }
    event_bus_.publish(PlaybackStartedEvent{});
}

void PlaybackEngine::pause() {
    {
        std::lock_guard<std::mutex> lock(control_mutex_);
        auto expected = PlayerState::Playing;
        if (!state_.compare_exchange_strong(expected, PlayerState::Paused))
            return;
        audio_clock_.pause();
        video_clock_.pause();
        if (audio_output_) audio_output_->pause();
    }
    event_bus_.publish(PlaybackPausedEvent{});
}

void PlaybackEngine::seek(double seconds) {
    std::lock_guard<std::mutex> lock(control_mutex_);
    if (state_.load() != PlayerState::Playing &&
        state_.load() != PlayerState::Paused) {
        return;
    }
    seek_target_ = std::max(0.0, seconds);
    seek_requested_ = true;
}

void PlaybackEngine::stop() {
    {
        std::lock_guard<std::mutex> lock(control_mutex_);
        running_ = false;
        if (audio_output_) {
            audio_output_->stop();
        }
    }
    video_packet_queue_.drain();
    audio_packet_queue_.drain();
    if (playback_thread_.joinable()) playback_thread_.join();
    if (video_decode_thread_.joinable()) video_decode_thread_.join();
    if (audio_decode_thread_.joinable()) audio_decode_thread_.join();
    {
        std::lock_guard<std::mutex> lock(control_mutex_);
        if (audio_output_) {
            audio_output_->shutdown();
        }
        flush_buffers();
        audio_clock_.reset();
        video_clock_.reset();
        video_clock_locked_ = false;
        set_state(PlayerState::Stopped);
    }
    event_bus_.publish(PlaybackEndedEvent{});
}

int PlaybackEngine::fill_audio_buffer(uint8_t* data, int frames_requested) {
    int total_frames = 0;
    int bytes_per_frame = 0;

    if (audio_stream_ >= 0) {
        AVSampleFormat fmt = AV_SAMPLE_FMT_FLT;
        bytes_per_frame = av_get_bytes_per_sample(fmt);
        if (bytes_per_frame <= 0) bytes_per_frame = 4;
        bytes_per_frame *= 2; // Forced stereo
    } else {
        bytes_per_frame = 8;
    }

    while (total_frames < frames_requested) {
        if (leftover_audio_frames_ <= 0) {
            if (!audio_queue_.try_pop(current_audio_frame_, 0)) break;
            if (current_audio_frame_.planar_data.empty()) continue;
            leftover_audio_frames_ = current_audio_frame_.nb_samples;
            leftover_audio_offset_ = 0;
            audio_clock_.set_pts(current_audio_frame_.pts);
        }

        int copy_frames = std::min(leftover_audio_frames_, frames_requested - total_frames);
        int copy_bytes = copy_frames * bytes_per_frame;

        std::memcpy(data + total_frames * bytes_per_frame,
                   current_audio_frame_.planar_data.data() + leftover_audio_offset_ * bytes_per_frame,
                   copy_bytes);

        total_frames += copy_frames;
        leftover_audio_frames_ -= copy_frames;
        leftover_audio_offset_ += copy_frames;
        audio_clock_.add_bytes_consumed(copy_bytes);
    }

    if (total_frames < frames_requested) {
        int silence_bytes = (frames_requested - total_frames) * bytes_per_frame;
        std::memset(data + total_frames * bytes_per_frame, 0, silence_bytes);
        audio_clock_.add_bytes_consumed(silence_bytes); // Advance clock even on silence to prevent freezing
        static int once = 0; if (++once <= 3) fprintf(stderr, "fill_audio_buffer: SILENCE filled %d/%d frames (aq size=%zu)\n",
              frames_requested - total_frames, frames_requested, audio_queue_.size());
    }

    return frames_requested;
}

bool PlaybackEngine::sync_pop_video_frame(VideoFrame& out) {
    if (!has_video()) return false;
    double current = current_time();

    static int diag_cnt = 0;
    const double drop_threshold = 0.3;
    while (true) {
        double front_pts = video_queue_.front_pts();
        if (front_pts < 0) {
            if (++diag_cnt % 30 == 1)
                fprintf(stderr, "sync_pop: front_pts < 0 (empty queue?)\n");
            return false;
        }
        // Only drop frames after the video clock has been locked by a popped frame.
        // Before that, video_clock_ is just wall-clock elapsed since startup, not
        // actual playback position.
        if (video_clock_locked_ && current - front_pts > drop_threshold) {
            VideoFrame dropped;
            if (!video_queue_.try_pop(dropped, 0)) break;
            if (++diag_cnt % 30 == 1)
                fprintf(stderr, "sync_pop: dropped frame pts=%.3f\n", dropped.pts);
            continue;
        }
        break;
    }

    double front_pts = video_queue_.front_pts();
    if (front_pts < 0) return false;
    if (front_pts <= current + 0.001) {
        if (video_queue_.try_pop(out, 0)) {
            video_clock_.set_pts(out.pts, 0);
            video_clock_locked_ = true;
            if (++diag_cnt % 30 == 1)
                fprintf(stderr, "sync_pop: popped frame pts=%.3f current=%.3f\n", out.pts, current);
            return true;
        }
    } else if (++diag_cnt % 30 == 1) {
        fprintf(stderr, "sync_pop: front_pts=%.3f > current=%.3f (not ready yet)\n", front_pts, current);
    }
    return false;
}

std::string PlaybackEngine::video_codec_name() {
    if (video_stream_ < 0) return {};
    auto* stream = reader_.context()->streams[video_stream_];
    return stream->codecpar ? avcodec_get_name(stream->codecpar->codec_id) : "";
}

std::string PlaybackEngine::audio_codec_name() {
    if (audio_stream_ < 0) return {};
    auto* stream = reader_.context()->streams[audio_stream_];
    return stream->codecpar ? avcodec_get_name(stream->codecpar->codec_id) : "";
}

int PlaybackEngine::video_width() const {
    if (video_rotation_ == 90 || video_rotation_ == 270) {
        return video_height_;
    }
    return video_width_;
}

int PlaybackEngine::video_height() const {
    if (video_rotation_ == 90 || video_rotation_ == 270) {
        return video_width_;
    }
    return video_height_;
}

void PlaybackEngine::set_state(PlayerState new_state) {
    PlayerState old = state_.exchange(new_state);
    if (old != new_state) {
        event_bus_.publish(StateChangedEvent{old, new_state});
    }
}

void PlaybackEngine::resume_internal() {
    audio_clock_.resume();
    video_clock_.resume();
    if (audio_output_) audio_output_->resume();
    set_state(PlayerState::Playing);
    event_bus_.publish(PlaybackStartedEvent{});
}

void PlaybackEngine::flush_buffers() {
    video_packet_queue_.reset();
    audio_packet_queue_.reset();
    video_queue_.reset();
    audio_queue_.reset();
    subtitle_queue_.clear();
    
    leftover_audio_frames_ = 0;
    leftover_audio_offset_ = 0;
    current_audio_frame_ = AudioFrame();
}

void PlaybackEngine::handle_seek() {
    seek_requested_ = false;
    flush_requested_ = true;
    set_state(PlayerState::Seeking);

    // Drain packet queues to unblock decode threads
    video_packet_queue_.drain();
    audio_packet_queue_.drain();

    // Wait for decode threads to notice flush_requested_
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    flush_buffers();
    video_decoder_.flush();
    audio_decoder_.flush();
    subtitle_decoder_.flush();

    flush_requested_ = false;
    int seek_stream = video_stream_ >= 0 ? video_stream_ : audio_stream_;
    reader_.seek_to_time(seek_target_, seek_stream);
    audio_clock_.set_pts(seek_target_);
    video_clock_.set_pts(seek_target_, 0);

    set_state(PlayerState::Playing);
}

void PlaybackEngine::demux_thread_fn() {
    AVPacket* pkt = av_packet_alloc();
    if (!pkt) { running_ = false; return; }

    bool eof_reached = false;

    while (running_) {
        if (seek_requested_) {
            handle_seek();
            eof_reached = false;
            continue;
        }

        if (state_.load() == PlayerState::Paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Throttle demuxing if packet queues are full
        if ((video_stream_ >= 0 && video_packet_queue_.size() >= 120) ||
            (audio_stream_ >= 0 && audio_packet_queue_.size() >= 120)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (audio_stream_ >= 0 && audio_output_ && audio_output_->is_initialized() && !audio_started_) {
            if (audio_output_->play()) {
                audio_started_ = true;
            }
        }

        if (!eof_reached) {
            int ret = av_read_frame(reader_.context(), pkt);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    eof_reached = true;
                    // Signal EOF to decode threads by draining their queues
                    video_packet_queue_.drain();
                    audio_packet_queue_.drain();
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
            }
        }

        if (!eof_reached) {
            auto packet = PacketPtr(av_packet_alloc());
            if (packet) {
                av_packet_move_ref(packet.get(), pkt);

                int si = packet->stream_index;
                if (si == video_stream_) {
                    video_packet_queue_.push(std::move(packet));
                } else if (si == audio_stream_) {
                    audio_packet_queue_.push(std::move(packet));
                } else if (si == subtitle_stream_) {
                    double tb = av_q2d(reader_.context()->streams[subtitle_stream_]->time_base);
                    auto subs = subtitle_decoder_.decode(packet.get(), tb);
                    for (auto& ds : subs) {
                        subtitle_queue_.push(std::move(ds.frame));
                    }
                }
            }
            av_packet_unref(pkt);
        }

        if (eof_reached) {
            // Check if both decode queues are done
            bool v_done = !video_packet_queue_.size() && video_queue_.empty();
            bool a_done = !audio_packet_queue_.size() && audio_queue_.empty();

            if (v_done && a_done) {
                running_ = false;
                if (audio_output_) {
                    audio_output_->stop();
                }
                set_state(PlayerState::Stopped);
                event_bus_.publish(PlaybackEndedEvent{});
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    av_packet_free(&pkt);
}

void PlaybackEngine::video_decode_thread_fn() {
    while (running_) {
        if (flush_requested_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        PacketPtr pkt;
        if (!video_packet_queue_.try_pop(pkt, 100)) {
            continue;
        }

        // nullptr packet signals flush/drain
        if (!pkt) continue;

        if (seek_requested_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (!video_decoder_.send_packet(pkt.get())) continue;

        while (running_ && !seek_requested_) {
            auto frame = video_decoder_.receive_frame();
            if (!frame) break;

            VideoFrame vf;
            vf.frame = std::move(frame);

            double pts = 0.0;
            if (vf.frame->pts != AV_NOPTS_VALUE) {
                pts = vf.frame->pts * av_q2d(reader_.context()->streams[video_stream_]->time_base);
            } else if (vf.frame->best_effort_timestamp != AV_NOPTS_VALUE) {
                pts = vf.frame->best_effort_timestamp * av_q2d(reader_.context()->streams[video_stream_]->time_base);
            }
            vf.pts = pts;
            vf.width = video_decoder_.width();
            vf.height = video_decoder_.height();
            vf.rotation = video_rotation_;
            vf.pix_fmt = video_decoder_.pixel_format();
            vf.format = VideoFrame::from_av_pixel_format(vf.pix_fmt);
            vf.is_10bit = (vf.format == VideoFrameFormat::YUV420P10);
            vf.is_hdr = (vf.frame->color_trc == AVCOL_TRC_SMPTE2084 || vf.frame->color_trc == AVCOL_TRC_ARIB_STD_B67);
            vf.is_bt2020 = (vf.frame->colorspace == AVCOL_SPC_BT2020_NCL || vf.frame->colorspace == AVCOL_SPC_BT2020_CL);
            vf.duration = 1.0 / 30.0;
            if (video_width_ == 0) {
                video_width_ = vf.width;
                video_height_ = vf.height;
            }
            // Throttle decode: pause when the queue is full to avoid dropping frames
            while (video_queue_.size() >= 30 && running_ && !seek_requested_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (!running_ || seek_requested_) break;
            
            video_queue_.push(std::move(vf));
        }
    }

    // Flush remaining frames from decoder
    video_decoder_.send_packet(nullptr);
    while (running_) {
        auto frame = video_decoder_.receive_frame();
        if (!frame) break;
        VideoFrame vf;
        vf.frame = std::move(frame);
        double pts = 0.0;
        if (vf.frame->pts != AV_NOPTS_VALUE) {
            pts = vf.frame->pts * av_q2d(reader_.context()->streams[video_stream_]->time_base);
        } else if (vf.frame->best_effort_timestamp != AV_NOPTS_VALUE) {
            pts = vf.frame->best_effort_timestamp * av_q2d(reader_.context()->streams[video_stream_]->time_base);
        }
        vf.pts = pts;
        vf.width = video_decoder_.width();
        vf.height = video_decoder_.height();
        vf.rotation = video_rotation_;
        vf.pix_fmt = video_decoder_.pixel_format();
        vf.format = VideoFrame::from_av_pixel_format(vf.pix_fmt);
        vf.is_10bit = (vf.format == VideoFrameFormat::YUV420P10);
        vf.is_hdr = (vf.frame->color_trc == AVCOL_TRC_SMPTE2084 || vf.frame->color_trc == AVCOL_TRC_ARIB_STD_B67);
        vf.is_bt2020 = (vf.frame->colorspace == AVCOL_SPC_BT2020_NCL || vf.frame->colorspace == AVCOL_SPC_BT2020_CL);
        vf.duration = 1.0 / 30.0;
        
        while (video_queue_.size() >= 30 && running_ && !seek_requested_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (!running_ || seek_requested_) break;
        
        video_queue_.push(std::move(vf));
    }
}

void PlaybackEngine::audio_decode_thread_fn() {
    while (running_) {
        if (flush_requested_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        PacketPtr pkt;
        if (!audio_packet_queue_.try_pop(pkt, 100)) {
            continue;
        }

        if (!pkt) continue;

        if (seek_requested_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (!audio_decoder_.send_packet(pkt.get())) continue;

        while (running_ && !seek_requested_) {
            auto frame = audio_decoder_.receive_frame();
            if (!frame) break;

            AudioFrame af;

            double pts = 0.0;
            if (frame->pts != AV_NOPTS_VALUE) {
                pts = frame->pts * av_q2d(reader_.context()->streams[audio_stream_]->time_base);
            } else if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
                pts = frame->best_effort_timestamp * av_q2d(reader_.context()->streams[audio_stream_]->time_base);
            }
            af.pts = pts;
            af.sample_rate = audio_decoder_.sample_rate();
            af.format = AV_SAMPLE_FMT_FLT;
            af.nb_samples = frame->nb_samples;

            af.channels = 2; // Forced stereo
            af.channel_layout = AV_CH_LAYOUT_STEREO;
            af.duration = AudioFrame::duration_from_frame(frame.get());

            while (audio_queue_.size() >= 30 && running_ && !seek_requested_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (!running_ || seek_requested_) break;

            if (resample_audio(frame.get(), af.planar_data)) {
                af.frame = std::move(frame);
                audio_queue_.push(std::move(af));
            }
        }
    }

    // Flush remaining frames
    audio_decoder_.send_packet(nullptr);
    while (running_) {
        auto frame = audio_decoder_.receive_frame();
        if (!frame) break;
        AudioFrame af;
        double pts = 0.0;
        if (frame->pts != AV_NOPTS_VALUE) {
            pts = frame->pts * av_q2d(reader_.context()->streams[audio_stream_]->time_base);
        } else if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
            pts = frame->best_effort_timestamp * av_q2d(reader_.context()->streams[audio_stream_]->time_base);
        }
        af.pts = pts;
        af.sample_rate = audio_decoder_.sample_rate();
        af.format = AV_SAMPLE_FMT_FLT;
        af.nb_samples = frame->nb_samples;
        af.channels = 2; // Forced stereo
        af.channel_layout = AV_CH_LAYOUT_STEREO;
        af.duration = AudioFrame::duration_from_frame(frame.get());
        
        while (audio_queue_.size() >= 30 && running_ && !seek_requested_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (!running_ || seek_requested_) break;
        
        if (resample_audio(frame.get(), af.planar_data)) {
            af.frame = std::move(frame);
            audio_queue_.push(std::move(af));
        }
    }
}

bool PlaybackEngine::resample_audio(const AVFrame* frame, std::vector<uint8_t>& out_buffer) {
    if (!frame) return false;

    bool params_changed = false;
    if (!swr_ctx_) {
        params_changed = true;
    } else {
        if (last_in_sample_fmt_ != frame->format ||
            last_in_sample_rate_ != frame->sample_rate ||
            av_channel_layout_compare(&last_in_ch_layout_, &frame->ch_layout) != 0) {
            params_changed = true;
        }
    }

    if (params_changed) {
        if (swr_ctx_) {
            swr_free(&swr_ctx_);
            swr_ctx_ = nullptr;
        }

        av_channel_layout_uninit(&last_in_ch_layout_);
        av_channel_layout_copy(&last_in_ch_layout_, &frame->ch_layout);
        last_in_sample_fmt_ = (AVSampleFormat)frame->format;
        last_in_sample_rate_ = frame->sample_rate;

        AVChannelLayout out_layout;
        av_channel_layout_default(&out_layout, 2); // Force stereo downmix

        int ret = swr_alloc_set_opts2(&swr_ctx_,
                                      &out_layout, AV_SAMPLE_FMT_FLT, frame->sample_rate,
                                      &frame->ch_layout, (AVSampleFormat)frame->format, frame->sample_rate,
                                      0, nullptr);
        av_channel_layout_uninit(&out_layout);

        if (ret < 0 || !swr_ctx_) {
            swr_ctx_ = nullptr;
            return false;
        }

        if (swr_init(swr_ctx_) < 0) {
            swr_free(&swr_ctx_);
            swr_ctx_ = nullptr;
            return false;
        }
    }

    int out_samples = frame->nb_samples;
    int out_channels = 2; // Force stereo
    int bytes_per_sample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
    int out_size = out_samples * out_channels * bytes_per_sample;
    out_buffer.resize(out_size);

    uint8_t* out_data[1] = { out_buffer.data() };
    int ret = swr_convert(swr_ctx_,
                          out_data, out_samples,
                          (const uint8_t**)frame->data, frame->nb_samples);
    if (ret < 0) {
        return false;
    }

    if (ret < out_samples) {
        out_buffer.resize(ret * out_channels * bytes_per_sample);
    }

    return true;
}

int PlaybackEngine::get_video_rotation() {
    if (video_stream_ < 0) return 0;
    auto* stream = reader_.context()->streams[video_stream_];

    const AVPacketSideData* sd = av_packet_side_data_get(
        stream->codecpar->coded_side_data, stream->codecpar->nb_coded_side_data, AV_PKT_DATA_DISPLAYMATRIX);
    if (sd && sd->data) {
        double rot = -av_display_rotation_get((const int32_t*)sd->data);
        int rotation = static_cast<int>(std::round(rot));
        if (rotation < 0) rotation += 360;
        return rotation;
    }

    AVDictionaryEntry *entry = av_dict_get(stream->metadata, "rotate", nullptr, 0);
    if (entry) {
        try {
            int rotation = std::stoi(entry->value);
            return (rotation % 360 + 360) % 360;
        } catch (...) {
            return 0;
        }
    }

    return 0;
}

} // namespace rav

#pragma once

#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/display.h>
}

#include "PlayerState.h"
#include "PlayerCommand.h"
#include "PlayerEvent.h"
#include "EventBus.h"
#include "../ffmpeg/ContainerReader.h"
#include "../ffmpeg/FFmpegContext.h"
#include "../decoder/VideoDecoder.h"
#include "../decoder/AudioDecoder.h"
#include "../video/VideoFrame.h"
#include "../video/FrameQueue.h"
#include "../video/VideoClock.h"
#include "../audio/AudioFrame.h"
#include "../audio/AudioQueue.h"
#include "../audio/AudioClock.h"
#include "../audio/AudioOutput.h"
#include "../video/VideoRenderer.h"

namespace rav {

class PlaybackEngine {
public:
    PlaybackEngine() = default;

    ~PlaybackEngine() { close(); }

    PlaybackEngine(const PlaybackEngine&) = delete;
    PlaybackEngine& operator=(const PlaybackEngine&) = delete;

    bool open(const std::string& url) {
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

        video_stream_ = -1;
        audio_stream_ = -1;

        for (const auto& s : info.streams) {
            if (s.type == AVMEDIA_TYPE_VIDEO && video_stream_ < 0)
                video_stream_ = s.index;
            if (s.type == AVMEDIA_TYPE_AUDIO && audio_stream_ < 0)
                audio_stream_ = s.index;
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

        if (video_stream_ < 0 && audio_stream_ < 0) {
            set_state(PlayerState::Error);
            return false;
        }

        event_bus_.publish(MediaLoadedEvent{duration_});
        set_state(PlayerState::Stopped);
        return true;
    }

    void close() {
        stop();
        reader_.close();
        video_decoder_.close();
        audio_decoder_.close();
        video_queue_.reset();
        audio_queue_.reset();
        leftover_audio_frames_ = 0;
        leftover_audio_offset_ = 0;
        current_audio_frame_ = AudioFrame();
        video_clock_.reset();
        audio_clock_.reset();
        duration_ = 0.0;
        video_stream_ = -1;
        audio_stream_ = -1;
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

    void play() {
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

        if (audio_output_ && audio_stream_ >= 0) {
            AudioOutputSpec spec;
            spec.sample_rate = audio_decoder_.sample_rate();
            spec.format = AV_SAMPLE_FMT_FLT; // Force float packed format
            auto* stream = reader_.context()->streams[audio_stream_];
            auto* par = stream->codecpar;
            spec.channels = par->ch_layout.nb_channels;
            spec.channel_layout = par->ch_layout.u.mask;
            audio_output_->init(spec);

            int bytes_per_sample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
            if (bytes_per_sample <= 0) bytes_per_sample = 4;
            audio_clock_.set_bytes_per_second(
                spec.sample_rate * spec.channels * bytes_per_sample);
        }

        set_state(PlayerState::Playing);
        running_ = true;
        playback_thread_ = std::thread(&PlaybackEngine::playback_thread_fn, this);
        event_bus_.publish(PlaybackStartedEvent{});
    }

    void pause() {
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

    void seek(double seconds) {
        std::lock_guard<std::mutex> lock(control_mutex_);
        if (state_.load() != PlayerState::Playing &&
            state_.load() != PlayerState::Paused) {
            return;
        }
        seek_target_ = std::max(0.0, seconds);
        seek_requested_ = true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(control_mutex_);
            running_ = false;
            if (audio_output_) {
                audio_output_->stop();
            }
        }
        if (playback_thread_.joinable()) {
            playback_thread_.join();
        }
        {
            std::lock_guard<std::mutex> lock(control_mutex_);
            if (audio_output_) {
                audio_output_->shutdown();
            }
            flush_buffers();
            audio_clock_.reset();
            video_clock_.reset();
            set_state(PlayerState::Stopped);
        }
        event_bus_.publish(PlaybackEndedEvent{});
    }

    PlayerState state() const { return state_.load(); }
    double duration() const { return duration_; }

    double current_time() const {
        if (audio_clock_.is_running()) {
            return audio_clock_.pts();
        }
        return video_clock_.pts();
    }

    void set_video_renderer(VideoRenderer* renderer) {
        video_renderer_ = renderer;
    }

    void set_audio_output(AudioOutput* output) {
        audio_output_ = output;
    }

    EventBus<PlayerCommand>& command_bus() { return command_bus_; }
    EventBus<PlayerEvent>& event_bus() { return event_bus_; }

    void set_volume(float vol) { volume_ = vol; }
    float volume() const { return volume_; }

    int fill_audio_buffer(uint8_t* data, int frames_requested) {
        int total_frames = 0;
        int bytes_per_frame = 0;

        if (audio_stream_ >= 0) {
            AVSampleFormat fmt = AV_SAMPLE_FMT_FLT;
            bytes_per_frame = av_get_bytes_per_sample(fmt);
            if (bytes_per_frame <= 0) bytes_per_frame = 4;
            auto* stream = reader_.context()->streams[audio_stream_];
            bytes_per_frame *= stream->codecpar->ch_layout.nb_channels;
        } else {
            bytes_per_frame = 8;
        }

        while (total_frames < frames_requested) {
            if (leftover_audio_frames_ <= 0) {
                if (!audio_queue_.try_pop(current_audio_frame_, 100)) break;
                if (current_audio_frame_.planar_data.empty()) continue;
                leftover_audio_frames_ = current_audio_frame_.nb_samples;
                leftover_audio_offset_ = 0;
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
        }

        return frames_requested;
    }

    bool has_video() const { return video_stream_ >= 0; }
    bool has_audio() const { return audio_stream_ >= 0; }
    int video_queue_size() const { return video_queue_.size(); }
    int audio_queue_size() const { return audio_queue_.size(); }

    bool sync_pop_video_frame(VideoFrame& out) {
        if (!has_video()) return false;
        double current = current_time();
        double front_pts = video_queue_.front_pts();
        if (front_pts < 0) return false;
        if (front_pts <= current + 0.001) {
            if (video_queue_.try_pop(out, 0)) {
                video_clock_.set_pts(out.pts, 0);
                return true;
            }
        }
        return false;
    }

    VideoFrame pop_video_frame() {
        return video_queue_.pop();
    }

    bool try_pop_video_frame(VideoFrame& out, int timeout_ms = 0) {
        return video_queue_.try_pop(out, timeout_ms);
    }

    int video_width() const {
        if (video_rotation_ == 90 || video_rotation_ == 270) {
            return video_height_;
        }
        return video_width_;
    }

    int video_height() const {
        if (video_rotation_ == 90 || video_rotation_ == 270) {
            return video_width_;
        }
        return video_height_;
    }

private:
    void set_state(PlayerState new_state) {
        PlayerState old = state_.exchange(new_state);
        if (old != new_state) {
            event_bus_.publish(StateChangedEvent{old, new_state});
        }
    }

    void resume_internal() {
        audio_clock_.resume();
        video_clock_.resume();
        if (audio_output_) audio_output_->resume();
        set_state(PlayerState::Playing);
        event_bus_.publish(PlaybackStartedEvent{});
    }

    void flush_buffers() {
        video_queue_.reset();
        audio_queue_.reset();
    }

    void handle_seek() {
        seek_requested_ = false;
        set_state(PlayerState::Seeking);

        flush_buffers();
        video_decoder_.close();
        audio_decoder_.close();

        if (video_stream_ >= 0) {
            auto* stream = reader_.context()->streams[video_stream_];
            video_decoder_.open(stream->codecpar);
        }
        if (audio_stream_ >= 0) {
            auto* stream = reader_.context()->streams[audio_stream_];
            audio_decoder_.open(stream->codecpar);
        }

        reader_.seek_to_time(seek_target_);
        audio_clock_.set_pts(seek_target_);
        video_clock_.set_pts(seek_target_, 0);

        set_state(PlayerState::Playing);
    }

    void playback_thread_fn() {
        AVPacket* pkt = av_packet_alloc();
        if (!pkt) { running_ = false; return; }

        bool eof_reached = false;
        bool video_flushed = false;
        bool audio_flushed = false;

        while (running_) {
            if (seek_requested_) {
                handle_seek();
                eof_reached = false;
                video_flushed = false;
                audio_flushed = false;
                continue;
            }

            if (state_.load() == PlayerState::Paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Rate limit decoding if queues are full
            if ((video_stream_ >= 0 && video_queue_.size() >= 30) ||
                (audio_stream_ >= 0 && audio_queue_.size() >= 30)) {
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
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                }
            }

            if (!eof_reached) {
                if (pkt->stream_index == video_stream_) {
                    if (video_decoder_.send_packet(pkt)) {
                        while (true) {
                            auto frame = video_decoder_.receive_frame();
                            if (!frame) break;

                            VideoFrame vf;
                            vf.frame = std::move(frame);

                            double pts = 0.0;
                            if (vf.frame->pts != AV_NOPTS_VALUE) {
                                pts = vf.frame->pts * av_q2d(reader_.context()->streams[video_stream_]->time_base);
                            } else if (vf.frame->best_effort_timestamp != AV_NOPTS_VALUE) {
                                pts = vf.frame->best_effort_timestamp * av_q2d(reader_.context()->streams[video_stream_]->time_base);
                            } else {
                                pts = pkt->pts * av_q2d(reader_.context()->streams[video_stream_]->time_base);
                            }
                            vf.pts = pts;

                            vf.width = video_decoder_.width();
                            vf.height = video_decoder_.height();
                            vf.rotation = video_rotation_;
                            vf.pix_fmt = video_decoder_.pixel_format();
                            vf.format = VideoFrame::from_av_pixel_format(vf.pix_fmt);
                            vf.duration = 1.0 / 30.0;
                            if (video_width_ == 0) {
                                video_width_ = vf.width;
                                video_height_ = vf.height;
                            }
                            video_queue_.push(std::move(vf));
                        }
                    }
                } else if (pkt->stream_index == audio_stream_) {
                    if (audio_decoder_.send_packet(pkt)) {
                        while (true) {
                            auto frame = audio_decoder_.receive_frame();
                            if (!frame) break;

                            AudioFrame af;

                            double pts = 0.0;
                            if (frame->pts != AV_NOPTS_VALUE) {
                                pts = frame->pts * av_q2d(reader_.context()->streams[audio_stream_]->time_base);
                            } else if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
                                pts = frame->best_effort_timestamp * av_q2d(reader_.context()->streams[audio_stream_]->time_base);
                            } else {
                                pts = pkt->pts * av_q2d(reader_.context()->streams[audio_stream_]->time_base);
                            }
                            af.pts = pts;

                            af.sample_rate = audio_decoder_.sample_rate();
                            af.format = AV_SAMPLE_FMT_FLT;
                            af.nb_samples = frame->nb_samples;

                            auto* par = reader_.context()->streams[audio_stream_]->codecpar;
                            af.channels = par->ch_layout.nb_channels;
                            af.channel_layout = par->ch_layout.u.mask;

                            af.duration = AudioFrame::duration_from_frame(frame.get());

                            if (resample_audio(frame.get(), af.planar_data)) {
                                af.frame = std::move(frame);
                                audio_queue_.push(std::move(af));
                            }
                        }
                    }
                }
                av_packet_unref(pkt);
            }

            if (eof_reached) {
                bool dynamic_work_done = false;

                if (video_stream_ >= 0 && !video_flushed) {
                    video_decoder_.send_packet(nullptr);
                    while (true) {
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
                        vf.duration = 1.0 / 30.0;
                        video_queue_.push(std::move(vf));
                        dynamic_work_done = true;
                    }
                    video_flushed = true;
                }

                if (audio_stream_ >= 0 && !audio_flushed) {
                    audio_decoder_.send_packet(nullptr);
                    while (true) {
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

                        auto* par = reader_.context()->streams[audio_stream_]->codecpar;
                        af.channels = par->ch_layout.nb_channels;
                        af.channel_layout = par->ch_layout.u.mask;
                        af.duration = AudioFrame::duration_from_frame(frame.get());

                        if (resample_audio(frame.get(), af.planar_data)) {
                            af.frame = std::move(frame);
                            audio_queue_.push(std::move(af));
                            dynamic_work_done = true;
                        }
                    }
                    audio_flushed = true;
                }

                bool queues_empty = true;
                if (video_stream_ >= 0 && !video_queue_.empty()) queues_empty = false;
                if (audio_stream_ >= 0 && !audio_queue_.empty()) queues_empty = false;

                if (queues_empty && video_flushed && audio_flushed) {
                    running_ = false;
                    if (audio_output_) {
                        audio_output_->stop();
                    }
                    set_state(PlayerState::Stopped);
                    event_bus_.publish(PlaybackEndedEvent{});
                    break;
                }

                if (!dynamic_work_done) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }

        av_packet_free(&pkt);
    }

    bool resample_audio(const AVFrame* frame, std::vector<uint8_t>& out_buffer) {
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
            av_channel_layout_copy(&out_layout, &frame->ch_layout);

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
        int channels = frame->ch_layout.nb_channels;
        int bytes_per_sample = av_get_bytes_per_sample(AV_SAMPLE_FMT_FLT);
        int out_size = out_samples * channels * bytes_per_sample;
        out_buffer.resize(out_size);

        uint8_t* out_data[1] = { out_buffer.data() };
        int ret = swr_convert(swr_ctx_,
                              out_data, out_samples,
                              (const uint8_t**)frame->data, frame->nb_samples);
        if (ret < 0) {
            return false;
        }

        if (ret < out_samples) {
            out_buffer.resize(ret * channels * bytes_per_sample);
        }

        return true;
    }

    EventBus<PlayerCommand> command_bus_;
    EventBus<PlayerEvent> event_bus_;

    ContainerReader reader_;
    VideoDecoder video_decoder_;
    AudioDecoder audio_decoder_;

    FrameQueue video_queue_{30};
    AudioQueue audio_queue_{30};

    AudioFrame current_audio_frame_;
    int leftover_audio_frames_{0};
    int leftover_audio_offset_{0};

    VideoClock video_clock_;
    AudioClock audio_clock_;

    VideoRenderer* video_renderer_{nullptr};
    AudioOutput* audio_output_{nullptr};

    int video_stream_{-1};
    int audio_stream_{-1};

    int video_width_{0};
    int video_height_{0};
    double duration_{0.0};
    std::atomic<PlayerState> state_{PlayerState::Idle};
    std::atomic<float> volume_{1.0f};

    std::mutex control_mutex_;
    std::thread playback_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> seek_requested_{false};
    bool audio_started_{false};
    double seek_target_{0.0};

    int get_video_rotation() {
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

    SwrContext* swr_ctx_{nullptr};
    AVSampleFormat last_in_sample_fmt_{AV_SAMPLE_FMT_NONE};
    int last_in_sample_rate_{0};
    AVChannelLayout last_in_ch_layout_{};
    int video_rotation_{0};
};

} // namespace rav

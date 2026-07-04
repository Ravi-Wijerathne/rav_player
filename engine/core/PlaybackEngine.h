#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
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

        set_state(PlayerState::Loading);

        reader_.close();
        video_decoder_.close();
        audio_decoder_.close();

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
        video_clock_.reset();
        audio_clock_.reset();
        duration_ = 0.0;
        video_stream_ = -1;
        audio_stream_ = -1;
        set_state(PlayerState::Idle);
    }

    void play() {
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
        video_clock_.reset();

        if (audio_output_) {
            AudioOutputSpec spec;
            if (audio_stream_ >= 0) {
                spec.sample_rate = audio_decoder_.sample_rate();
                spec.format = audio_decoder_.sample_format();
                auto* stream = reader_.context()->streams[audio_stream_];
                auto* par = stream->codecpar;
                spec.channels = par->ch_layout.nb_channels;
                spec.channel_layout = par->ch_layout.u.mask;
            }
            audio_output_->init(spec);
        }

        set_state(PlayerState::Playing);
        running_ = true;
        playback_thread_ = std::thread(&PlaybackEngine::playback_thread_fn, this);
        event_bus_.publish(PlaybackStartedEvent{});
    }

    void pause() {
        auto expected = PlayerState::Playing;
        if (!state_.compare_exchange_strong(expected, PlayerState::Paused))
            return;
        audio_clock_.pause();
        if (audio_output_) audio_output_->pause();
        event_bus_.publish(PlaybackPausedEvent{});
    }

    void seek(double seconds) {
        if (state_.load() != PlayerState::Playing &&
            state_.load() != PlayerState::Paused) {
            return;
        }
        seek_target_ = std::max(0.0, seconds);
        seek_requested_ = true;
    }

    void stop() {
        running_ = false;
        if (playback_thread_.joinable()) {
            playback_thread_.join();
        }
        if (audio_output_) {
            audio_output_->stop();
            audio_output_->shutdown();
        }
        flush_buffers();
        audio_clock_.reset();
        video_clock_.reset();
        set_state(PlayerState::Stopped);
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
            AVSampleFormat fmt = audio_decoder_.sample_format();
            if (fmt == AV_SAMPLE_FMT_NONE) fmt = AV_SAMPLE_FMT_FLT;
            bytes_per_frame = av_get_bytes_per_sample(fmt);
            if (bytes_per_frame <= 0) bytes_per_frame = 4;
            auto* stream = reader_.context()->streams[audio_stream_];
            bytes_per_frame *= stream->codecpar->ch_layout.nb_channels;
        } else {
            bytes_per_frame = 8;
        }

        while (total_frames < frames_requested) {
            AudioFrame aframe = audio_queue_.pop();
            if (!aframe.frame) break;

            int frames = aframe.nb_samples;
            int copy_frames = std::min(frames, frames_requested - total_frames);
            int copy_bytes = copy_frames * bytes_per_frame;

            if (aframe.planar_data.empty()) {
                std::memcpy(data + total_frames * bytes_per_frame,
                           aframe.frame->data[0],
                           copy_bytes);
            } else {
                std::memcpy(data + total_frames * bytes_per_frame,
                           aframe.planar_data.data(),
                           copy_bytes);
            }

            total_frames += copy_frames;
            audio_clock_.add_bytes_consumed(copy_bytes);
        }

        if (total_frames < frames_requested) {
            std::memset(data + total_frames * bytes_per_frame, 0,
                       (frames_requested - total_frames) * bytes_per_frame);
        }

        return total_frames;
    }

    bool has_video() const { return video_stream_ >= 0; }
    bool has_audio() const { return audio_stream_ >= 0; }

    VideoFrame pop_video_frame() {
        return video_queue_.pop();
    }

    bool try_pop_video_frame(VideoFrame& out, int timeout_ms = 0) {
        return video_queue_.try_pop(out, timeout_ms);
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

        auto last_stats = std::chrono::steady_clock::now();

        while (running_) {
            if (seek_requested_) {
                handle_seek();
                continue;
            }

            if (state_.load() == PlayerState::Paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            if (audio_stream_ >= 0 && audio_output_ && audio_output_->is_initialized()) {
                if (!audio_output_->is_playing()) {
                    audio_output_->play();
                }
            }

            int ret = av_read_frame(reader_.context(), pkt);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    video_queue_.drain();
                    audio_queue_.drain();
                    continue;
                }
                continue;
            }

            if (pkt->stream_index == video_stream_) {
                if (video_decoder_.send_packet(pkt)) {
                    while (true) {
                        auto frame = video_decoder_.receive_frame();
                        if (!frame) break;

                        VideoFrame vf;
                        vf.frame = std::move(frame);
                        vf.pts = pkt->pts * av_q2d(
                            reader_.context()->streams[video_stream_]->time_base);
                        vf.width = video_decoder_.width();
                        vf.height = video_decoder_.height();
                        vf.pix_fmt = video_decoder_.pixel_format();
                        vf.format = VideoFrame::from_av_pixel_format(vf.pix_fmt);
                        vf.duration = 1.0 / 30.0;
                        video_queue_.push(std::move(vf));
                    }
                }
            } else if (pkt->stream_index == audio_stream_) {
                if (audio_decoder_.send_packet(pkt)) {
                    while (true) {
                        auto frame = audio_decoder_.receive_frame();
                        if (!frame) break;

                        AudioFrame af;
                        af.frame = std::move(frame);
                        af.pts = pkt->pts * av_q2d(
                            reader_.context()->streams[audio_stream_]->time_base);
                        af.sample_rate = audio_decoder_.sample_rate();
                        af.format = audio_decoder_.sample_format();
                        af.nb_samples = af.frame->nb_samples;

                        auto* par = reader_.context()->streams[audio_stream_]->codecpar;
                        af.channels = par->ch_layout.nb_channels;
                        af.channel_layout = par->ch_layout.u.mask;

                        af.duration = AudioFrame::duration_from_frame(af.frame.get());
                        audio_queue_.push(std::move(af));
                    }
                }
            }

            av_packet_unref(pkt);

            auto now = std::chrono::steady_clock::now();
            if (now - last_stats > std::chrono::seconds(1)) {
                last_stats = now;
            }

            if (state_.load() == PlayerState::Playing && video_renderer_ && video_renderer_->is_ready()) {
                VideoFrame vf;
                if (video_queue_.try_pop(vf, 0)) {
                    video_clock_.set_pts(vf.pts, 0);
                    video_renderer_->present_frame(vf);
                }
            }
        }

        av_packet_free(&pkt);
    }

    EventBus<PlayerCommand> command_bus_;
    EventBus<PlayerEvent> event_bus_;

    ContainerReader reader_;
    VideoDecoder video_decoder_;
    AudioDecoder audio_decoder_;

    FrameQueue video_queue_{30};
    AudioQueue audio_queue_{30};

    VideoClock video_clock_;
    AudioClock audio_clock_;

    VideoRenderer* video_renderer_{nullptr};
    AudioOutput* audio_output_{nullptr};

    int video_stream_{-1};
    int audio_stream_{-1};

    double duration_{0.0};
    std::atomic<PlayerState> state_{PlayerState::Idle};
    std::atomic<float> volume_{1.0f};

    std::thread playback_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> seek_requested_{false};
    double seek_target_{0.0};
};

} // namespace rav

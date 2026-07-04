#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "../ffmpeg/FFmpegContext.h"

namespace rav {

class AudioDecoder {
public:
    AudioDecoder() = default;

    ~AudioDecoder() = default;

    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;

    AudioDecoder(AudioDecoder&&) = default;
    AudioDecoder& operator=(AudioDecoder&&) = default;

    bool open(AVCodecParameters* codecpar,
              const AVCodec* codec = nullptr) {
        if (!codecpar) return false;
        if (!codec) {
            codec = avcodec_find_decoder(codecpar->codec_id);
        }
        if (!codec) return false;

        codec_ctx_.reset(avcodec_alloc_context3(codec));
        if (!codec_ctx_) return false;

        int ret = avcodec_parameters_to_context(codec_ctx_.get(), codecpar);
        if (ret < 0) return false;

        ret = avcodec_open2(codec_ctx_.get(), codec, nullptr);
        return ret >= 0;
    }

    void close() { codec_ctx_.reset(); }

    bool is_open() const { return codec_ctx_ != nullptr; }

    bool send_packet(AVPacket* pkt) {
        if (!codec_ctx_) return false;
        return avcodec_send_packet(codec_ctx_.get(), pkt) >= 0;
    }

    FramePtr receive_frame() {
        if (!codec_ctx_) return nullptr;
        auto frame = FramePtr(av_frame_alloc());
        if (!frame) return nullptr;

        int ret = avcodec_receive_frame(codec_ctx_.get(), frame.get());
        if (ret < 0) return nullptr;

        return frame;
    }

    AVCodecContext* context() { return codec_ctx_.get(); }

    int sample_rate() const {
        return codec_ctx_ ? codec_ctx_->sample_rate : 0;
    }

    AVSampleFormat sample_format() const {
        return codec_ctx_ ? codec_ctx_->sample_fmt : AV_SAMPLE_FMT_NONE;
    }

private:
    CodecContextPtr codec_ctx_;
};

} // namespace rav

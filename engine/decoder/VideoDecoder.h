#pragma once

#include <memory>
#include <optional>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "../ffmpeg/FFmpegContext.h"
#include "HardwareDecoder.h"

namespace rav {

class VideoDecoder {
public:
    VideoDecoder() = default;

    ~VideoDecoder() = default;

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    VideoDecoder(VideoDecoder&&) = default;
    VideoDecoder& operator=(VideoDecoder&&) = default;

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

        if (hw_decoder_) {
            hw_decoder_->init(codec_ctx_.get());
        }

        ret = avcodec_open2(codec_ctx_.get(), codec, nullptr);
        return ret >= 0;
    }

    void close() {
        codec_ctx_.reset();
        hw_decoder_.reset();
    }

    bool is_open() const { return codec_ctx_ != nullptr; }

    bool send_packet(AVPacket* pkt) {
        if (!codec_ctx_) return false;
        int ret = avcodec_send_packet(codec_ctx_.get(), pkt);
        return ret >= 0;
    }

    FramePtr receive_frame() {
        if (!codec_ctx_) return nullptr;
        auto frame = FramePtr(av_frame_alloc());
        if (!frame) return nullptr;

        int ret = avcodec_receive_frame(codec_ctx_.get(), frame.get());
        if (ret < 0) return nullptr;

        return frame;
    }

    void set_hardware_decoder(std::unique_ptr<HardwareDecoder> hw) {
        hw_decoder_ = std::move(hw);
    }

    AVCodecContext* context() { return codec_ctx_.get(); }

    int width() const {
        return codec_ctx_ ? codec_ctx_->width : 0;
    }

    int height() const {
        return codec_ctx_ ? codec_ctx_->height : 0;
    }

    AVPixelFormat pixel_format() const {
        return codec_ctx_ ? codec_ctx_->pix_fmt : AV_PIX_FMT_NONE;
    }

private:
    CodecContextPtr codec_ctx_;
    std::unique_ptr<HardwareDecoder> hw_decoder_;
};

} // namespace rav

#pragma once

#include <cstdint>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

#include "../ffmpeg/FFmpegContext.h"

namespace rav {

struct FrameConverterSpec {
    int src_width{0};
    int src_height{0};
    AVPixelFormat src_format{AV_PIX_FMT_NONE};

    int dst_width{0};
    int dst_height{0};
    AVPixelFormat dst_format{AV_PIX_FMT_RGBA};

    int flags{SWS_BILINEAR};
};

class FrameConverter {
public:
    FrameConverter() = default;

    ~FrameConverter() { close(); }

    FrameConverter(const FrameConverter&) = delete;
    FrameConverter& operator=(const FrameConverter&) = delete;

    FrameConverter(FrameConverter&& other) noexcept
        : sws_ctx_(other.sws_ctx_)
        , spec_(other.spec_)
        , dst_linesize_(other.dst_linesize_)
        , dst_buffer_(std::move(other.dst_buffer_))
        , is_open_(other.is_open_) {
        other.sws_ctx_ = nullptr;
        other.is_open_ = false;
    }

    FrameConverter& operator=(FrameConverter&& other) noexcept {
        if (this != &other) {
            close();
            sws_ctx_ = other.sws_ctx_;
            spec_ = other.spec_;
            dst_linesize_ = other.dst_linesize_;
            dst_buffer_ = std::move(other.dst_buffer_);
            is_open_ = other.is_open_;
            other.sws_ctx_ = nullptr;
            other.is_open_ = false;
        }
        return *this;
    }

    bool init(const FrameConverterSpec& spec) {
        close();

        if (spec.src_width <= 0 || spec.src_height <= 0 ||
            spec.dst_width <= 0 || spec.dst_height <= 0 ||
            spec.src_format == AV_PIX_FMT_NONE ||
            spec.dst_format == AV_PIX_FMT_NONE) {
            return false;
        }

        spec_ = spec;

        sws_ctx_ = sws_getContext(
            spec.src_width, spec.src_height, spec.src_format,
            spec.dst_width, spec.dst_height, spec.dst_format,
            spec.flags, nullptr, nullptr, nullptr);

        if (!sws_ctx_) return false;

        dst_linesize_ = spec.dst_width * 4;
        dst_buffer_.resize(static_cast<size_t>(dst_linesize_ * spec.dst_height));
        is_open_ = true;
        return true;
    }

    bool convert(const AVFrame* src, uint8_t* dst, int dst_stride) {
        if (!is_open_ || !src || !dst) return false;

        uint8_t* dst_slice[] = {dst};
        int dst_stride_arr[] = {dst_stride};

        int ret = sws_scale(sws_ctx_,
                            src->data, src->linesize, 0, src->height,
                            dst_slice, dst_stride_arr);

        return ret > 0;
    }

    bool convert_to_buffer(const AVFrame* src) {
        return convert(src, dst_buffer_.data(), dst_linesize_);
    }

    const uint8_t* data() const { return dst_buffer_.data(); }
    uint8_t* data() { return dst_buffer_.data(); }
    size_t data_size() const { return dst_buffer_.size(); }
    int linesize() const { return dst_linesize_; }

    const FrameConverterSpec& spec() const { return spec_; }
    bool is_open() const { return is_open_; }

    void close() {
        if (sws_ctx_) {
            sws_freeContext(sws_ctx_);
            sws_ctx_ = nullptr;
        }
        dst_buffer_.clear();
        is_open_ = false;
    }

private:
    SwsContext* sws_ctx_{nullptr};
    FrameConverterSpec spec_;
    int dst_linesize_{0};
    std::vector<uint8_t> dst_buffer_;
    bool is_open_{false};
};

} // namespace rav

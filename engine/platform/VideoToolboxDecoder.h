#pragma once

#if defined(__APPLE__)

#include "decoder/HardwareDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_videotoolbox.h>
}

namespace rav {

class VideoToolboxDecoder : public HardwareDecoder {
public:
    bool init(AVCodecContext* codec_ctx) override {
        if (!codec_ctx) return false;

        AVBufferRef* hw_device_ctx = nullptr;
        int ret = av_hwdevice_ctx_create(
            &hw_device_ctx, AV_HWDEVICE_TYPE_VIDEOTOOLBOX, nullptr, nullptr, 0);
        if (ret < 0) return false;

        codec_ctx->hw_device_ctx = hw_device_ctx;
        codec_ctx->hw_pix_fmt = AV_PIX_FMT_VIDEOTOOLBOX;
        return true;
    }

    HardwareDecoderType type() const override {
        return HardwareDecoderType::VideoToolbox;
    }

    AVHWDeviceType hw_device_type() const override {
        return AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
    }

    bool is_available() const override {
        AVBufferRef* test = nullptr;
        int ret = av_hwdevice_ctx_create(
            &test, AV_HWDEVICE_TYPE_VIDEOTOOLBOX, nullptr, nullptr, 0);
        if (ret >= 0) {
            av_buffer_unref(&test);
            return true;
        }
        return false;
    }
};

} // namespace rav

#endif // __APPLE__

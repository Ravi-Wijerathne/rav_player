#pragma once

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
}

#include "../ffmpeg/FFmpegContext.h"

namespace rav {

enum class HardwareDecoderType {
    None,
    VideoToolbox,
    D3D11VA,
    VAAPI,
    MediaCodec,
    Vulkan
};

class HardwareDecoder {
public:
    virtual ~HardwareDecoder() = default;

    virtual bool init(AVCodecContext* codec_ctx) = 0;
    virtual HardwareDecoderType type() const = 0;
    virtual AVHWDeviceType hw_device_type() const = 0;
    virtual bool is_available() const = 0;

    static bool is_hw_pixel_format(AVPixelFormat fmt) {
        switch (fmt) {
            case AV_PIX_FMT_VIDEOTOOLBOX:
            case AV_PIX_FMT_D3D11:
            case AV_PIX_FMT_VAAPI:
            case AV_PIX_FMT_MEDIACODEC:
            case AV_PIX_FMT_VULKAN:
                return true;
            default:
                return false;
        }
    }
};

} // namespace rav

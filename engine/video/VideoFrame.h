#pragma once

#include <memory>

#include "../ffmpeg/FFmpegContext.h"
#include "../decoder/HardwareDecoder.h"

namespace rav {

enum class VideoFrameFormat {
    Unknown,
    YUV420P,
    YUV420P10,
    NV12,
    YUV422,
    RGB,
    RGBA,
    Hardware
};

struct VideoFrame {
    FramePtr frame;
    double pts{0.0};
    double duration{0.0};
    int width{0};
    int height{0};
    int rotation{0};
    VideoFrameFormat format{VideoFrameFormat::Unknown};
    AVPixelFormat pix_fmt{AV_PIX_FMT_NONE};
    bool is_hardware{false};

    static VideoFrameFormat from_av_pixel_format(AVPixelFormat fmt) {
        switch (fmt) {
            case AV_PIX_FMT_YUV420P: return VideoFrameFormat::YUV420P;
            case AV_PIX_FMT_YUV420P10LE:
            case AV_PIX_FMT_YUV420P10BE: return VideoFrameFormat::YUV420P10;
            case AV_PIX_FMT_NV12:    return VideoFrameFormat::NV12;
            case AV_PIX_FMT_YUV422P: return VideoFrameFormat::YUV422;
            case AV_PIX_FMT_RGB24:   return VideoFrameFormat::RGB;
            case AV_PIX_FMT_RGBA:    return VideoFrameFormat::RGBA;
            default:
                if (HardwareDecoder::is_hw_pixel_format(fmt))
                    return VideoFrameFormat::Hardware;
                return VideoFrameFormat::Unknown;
        }
    }
};

} // namespace rav

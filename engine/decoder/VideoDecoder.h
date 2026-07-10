#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <mutex>

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

    VideoDecoder(VideoDecoder&&) = delete;
    VideoDecoder& operator=(VideoDecoder&&) = delete;

    bool open(AVCodecParameters* codecpar, const AVCodec* codec = nullptr);
    void flush();
    void close();
    bool is_open() const;
    bool send_packet(AVPacket* pkt);
    FramePtr receive_frame();
    void set_hardware_decoder(std::unique_ptr<HardwareDecoder> hw);
    
    AVCodecContext* context();
    int width() const;
    int height() const;
    AVPixelFormat pixel_format() const;

    CodecContextPtr codec_ctx_;
    std::unique_ptr<HardwareDecoder> hw_decoder_;
    std::mutex codec_mutex_;
};

} // namespace rav

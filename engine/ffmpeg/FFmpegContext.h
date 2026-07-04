#pragma once

#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

namespace rav {

struct FFmpegContextDeleter {
    void operator()(AVFormatContext* ctx) const {
        if (ctx) avformat_close_input(&ctx);
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) const {
        if (ctx) avcodec_free_context(&ctx);
    }
};

struct AVFrameDeleter {
    void operator()(AVFrame* frame) const {
        if (frame) av_frame_free(&frame);
    }
};

struct AVPacketDeleter {
    void operator()(AVPacket* pkt) const {
        if (pkt) av_packet_free(&pkt);
    }
};

using FormatContextPtr = std::unique_ptr<AVFormatContext, FFmpegContextDeleter>;
using CodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using FramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
using PacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;

inline bool ffmpeg_initialized() {
    static bool once = [] {
        avformat_network_init();
        return true;
    }();
    (void)once;
    return true;
}

} // namespace rav

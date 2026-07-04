#pragma once

#include <optional>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "FFmpegContext.h"

namespace rav {

struct CodecInfo {
    std::string name;
    std::string long_name;
    AVCodecID id;
    AVMediaType media_type;
    bool is_hardware_accelerated{false};
};

class CodecDiscovery {
public:
    static const AVCodec* find_decoder(AVCodecID codec_id) {
        return avcodec_find_decoder(codec_id);
    }

    static const AVCodec* find_decoder_by_name(const std::string& name) {
        return avcodec_find_decoder_by_name(name.c_str());
    }

    static std::optional<CodecInfo> describe_codec(const AVCodec* codec) {
        if (!codec) return std::nullopt;
        return CodecInfo{
            .name = codec->name ? codec->name : "",
            .long_name = codec->long_name ? codec->long_name : "",
            .id = codec->id,
            .media_type = codec->type
        };
    }

    static const AVCodec* find_best_decoder(AVFormatContext* fmt_ctx,
                                            AVMediaType type) {
        if (!fmt_ctx) return nullptr;
        int stream_index = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
        if (stream_index < 0) return nullptr;
        auto* codec = avcodec_find_decoder(
            fmt_ctx->streams[stream_index]->codecpar->codec_id);
        return codec;
    }
};

} // namespace rav

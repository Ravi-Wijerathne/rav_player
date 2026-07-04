#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include "FFmpegContext.h"

namespace rav {

struct StreamInfo {
    int index{-1};
    AVMediaType type{AVMEDIA_TYPE_UNKNOWN};
    AVCodecID codec_id{AV_CODEC_ID_NONE};
    std::string codec_name;
    int64_t bit_rate{0};

    int width{0};
    int height{0};
    AVRational framerate{};

    int sample_rate{0};
    AVChannelLayout channel_layout{};

    AVRational time_base{};
    int64_t duration{0};
};

struct ContainerInfo {
    std::string filename;
    std::string format_name;
    int64_t duration{0};
    int64_t bit_rate{0};
    std::vector<StreamInfo> streams;
    bool has_video{false};
    bool has_audio{false};
    bool has_subtitles{false};
};

class ContainerReader {
public:
    ContainerReader() { ffmpeg_initialized(); }

    ~ContainerReader() = default;

    ContainerReader(const ContainerReader&) = delete;
    ContainerReader& operator=(const ContainerReader&) = delete;

    ContainerReader(ContainerReader&&) = default;
    ContainerReader& operator=(ContainerReader&&) = default;

    bool open(const std::string& url) {
        AVFormatContext* ctx = nullptr;
        int ret = avformat_open_input(&ctx, url.c_str(), nullptr, nullptr);
        if (ret < 0 || !ctx) return false;

        fmt_ctx_.reset(ctx);

        ret = avformat_find_stream_info(fmt_ctx_.get(), nullptr);
        if (ret < 0) return false;

        build_info();
        return true;
    }

    void close() {
        fmt_ctx_.reset();
        info_.reset();
    }

    bool is_open() const { return fmt_ctx_ != nullptr; }

    const ContainerInfo& info() const {
        static ContainerInfo empty{};
        return info_ ? *info_ : empty;
    }

    AVFormatContext* context() { return fmt_ctx_.get(); }

    PacketPtr read_packet() {
        if (!fmt_ctx_) return nullptr;

        auto pkt = PacketPtr(av_packet_alloc());
        if (!pkt) return nullptr;

        int ret = av_read_frame(fmt_ctx_.get(), pkt.get());
        if (ret < 0) return nullptr;

        return pkt;
    }

    bool seek(int stream_index, int64_t timestamp, int flags = 0) {
        int ret = av_seek_frame(fmt_ctx_.get(), stream_index, timestamp, flags);
        return ret >= 0;
    }

    bool seek_to_time(double seconds, int stream_index = -1) {
        if (!fmt_ctx_) return false;
        int64_t ts = static_cast<int64_t>(seconds / av_q2d(
            stream_index >= 0
                ? fmt_ctx_->streams[stream_index]->time_base
                : av_make_q(1, AV_TIME_BASE)));
        return seek(stream_index, ts);
    }

private:
    void build_info() {
        if (!fmt_ctx_) return;

        auto ci = std::make_unique<ContainerInfo>();
        ci->filename = fmt_ctx_->url ? fmt_ctx_->url : "";
        ci->format_name = fmt_ctx_->iformat->name ? fmt_ctx_->iformat->name : "";
        ci->duration = fmt_ctx_->duration;
        ci->bit_rate = fmt_ctx_->bit_rate;

        for (unsigned i = 0; i < fmt_ctx_->nb_streams; ++i) {
            auto* stream = fmt_ctx_->streams[i];
            auto* par = stream->codecpar;
            if (!par) continue;

            StreamInfo si;
            si.index = i;
            si.type = par->codec_type;
            si.codec_id = par->codec_id;
            si.bit_rate = par->bit_rate;
            si.time_base = stream->time_base;
            si.duration = stream->duration;

            auto* codec = avcodec_find_decoder(par->codec_id);
            if (codec && codec->name) si.codec_name = codec->name;

            if (par->codec_type == AVMEDIA_TYPE_VIDEO) {
                si.width = par->width;
                si.height = par->height;
                si.framerate = stream->avg_frame_rate;
                ci->has_video = true;
            } else if (par->codec_type == AVMEDIA_TYPE_AUDIO) {
                si.sample_rate = par->sample_rate;
                av_channel_layout_copy(&si.channel_layout, &par->ch_layout);
                ci->has_audio = true;
            } else if (par->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                ci->has_subtitles = true;
            }

            ci->streams.push_back(std::move(si));
        }

        info_ = std::move(ci);
    }

    FormatContextPtr fmt_ctx_;
    std::unique_ptr<ContainerInfo> info_;
    PacketPtr current_packet_;
};

} // namespace rav

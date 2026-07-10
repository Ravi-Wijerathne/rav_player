#include "ContainerReader.h"

namespace rav {

ContainerReader::ContainerReader() { ffmpeg_initialized(); }

bool ContainerReader::open(const std::string& url) {
    AVFormatContext* ctx = nullptr;
    AVDictionary* options = nullptr;
    
    if (url.find("rtsp://") == 0) {
        av_dict_set(&options, "rtsp_transport", "tcp", 0);
        av_dict_set(&options, "timeout", "5000000", 0);
    } else if (url.find(".ts") != std::string::npos || url.find(".m2ts") != std::string::npos) {
        av_dict_set(&options, "probesize", "50000000", 0);
        av_dict_set(&options, "analyzeduration", "10000000", 0);
    }

    int ret = avformat_open_input(&ctx, url.c_str(), nullptr, &options);
    av_dict_free(&options);
    if (ret < 0 || !ctx) return false;

    fmt_ctx_.reset(ctx);

    ret = avformat_find_stream_info(fmt_ctx_.get(), nullptr);
    if (ret < 0) return false;

    build_info();
    return true;
}

void ContainerReader::close() {
    fmt_ctx_.reset();
    info_.reset();
}

bool ContainerReader::is_open() const { return fmt_ctx_ != nullptr; }

const ContainerInfo& ContainerReader::info() const {
    static ContainerInfo empty{};
    return info_ ? *info_ : empty;
}

AVFormatContext* ContainerReader::context() { return fmt_ctx_.get(); }

PacketPtr ContainerReader::read_packet() {
    if (!fmt_ctx_) return nullptr;

    auto pkt = PacketPtr(av_packet_alloc());
    if (!pkt) return nullptr;

    int ret = av_read_frame(fmt_ctx_.get(), pkt.get());
    if (ret < 0) return nullptr;

    return pkt;
}

bool ContainerReader::seek(int stream_index, int64_t timestamp, int flags) {
    int ret = av_seek_frame(fmt_ctx_.get(), stream_index, timestamp, flags);
    return ret >= 0;
}

bool ContainerReader::seek_to_time(double seconds, int stream_index) {
    if (!fmt_ctx_) return false;
    int64_t ts = static_cast<int64_t>(seconds / av_q2d(
        stream_index >= 0
            ? fmt_ctx_->streams[stream_index]->time_base
            : av_make_q(1, AV_TIME_BASE)));
    
    if (stream_index >= 0 && fmt_ctx_->streams[stream_index]->start_time != AV_NOPTS_VALUE) {
        ts += fmt_ctx_->streams[stream_index]->start_time;
    }
    
    fprintf(stderr, "seek_to_time: seconds=%.3f stream_index=%d ts=%lld\n", seconds, stream_index, (long long)ts);
    return seek(stream_index, ts, AVSEEK_FLAG_BACKWARD);
}

void ContainerReader::build_info() {
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

} // namespace rav

#include "MetadataExtractor.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace rav {

MediaInfo MetadataExtractor::extract(const std::string& file_path) {
    MediaInfo info;
    info.file_path = file_path;

    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, file_path.c_str(), nullptr, nullptr) != 0) {
        return info;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        return info;
    }

    info = extract_from_format(fmt_ctx);
    avformat_close_input(&fmt_ctx);
    return info;
}

MediaInfo MetadataExtractor::extract(const std::string& uri,
                                     const std::vector<uint8_t>& header_data) {
    (void)header_data;
    MediaInfo info;
    info.file_path = uri;

    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, uri.c_str(), nullptr, nullptr) != 0) {
        return info;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        return info;
    }

    info = extract_from_format(fmt_ctx);
    avformat_close_input(&fmt_ctx);
    return info;
}

bool MetadataExtractor::has_embedded_metadata(const std::string& file_path) {
    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, file_path.c_str(), nullptr, nullptr) != 0) {
        return false;
    }
    bool has_meta = fmt_ctx->metadata != nullptr &&
                    av_dict_count(fmt_ctx->metadata) > 0;
    avformat_close_input(&fmt_ctx);
    return has_meta;
}

MediaInfo MetadataExtractor::extract_from_format(void* fmt_ctx_ptr) {
    auto* fmt_ctx = static_cast<AVFormatContext*>(fmt_ctx_ptr);
    MediaInfo info;

    if (!fmt_ctx) return info;

    if (fmt_ctx->iformat) {
        info.format_name = fmt_ctx->iformat->name ? fmt_ctx->iformat->name : "";
        info.format_long_name = fmt_ctx->iformat->long_name ? fmt_ctx->iformat->long_name : "";
    }

    if (fmt_ctx->duration > 0) {
        info.duration = std::chrono::milliseconds(fmt_ctx->duration / 1000);
    }

    info.bit_rate = fmt_ctx->bit_rate;

    if (fmt_ctx->pb) {
        info.file_size = avio_size(fmt_ctx->pb);
    }

    extract_metadata_tags(fmt_ctx, info);
    extract_stream_info(fmt_ctx, info);

    return info;
}

void MetadataExtractor::extract_metadata_tags(void* fmt_ctx_ptr, MediaInfo& info) {
    auto* fmt_ctx = static_cast<AVFormatContext*>(fmt_ctx_ptr);
    if (!fmt_ctx || !fmt_ctx->metadata) return;

    AVDictionaryEntry* tag = nullptr;
    while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        std::string key(tag->key);
        std::string val(tag->value);

        if (key == "title") info.title = val;
        else if (key == "artist") info.artist = val;
        else if (key == "album") info.album = val;
        else if (key == "album_artist") info.album_artist = val;
        else if (key == "genre") info.genre = val;
        else if (key == "composer") info.composer = val;
        else if (key == "date" || key == "year") info.year = val;
        else if (key == "track") {
            try { info.track_number = std::stoi(val); } catch (...) {}
        }
        else if (key == "disc") {
            try { info.disc_number = std::stoi(val); } catch (...) {}
        }
        else if (key == "comment") info.comment = val;
        else if (key == "copyright") info.copyright = val;
        else if (key == "encoder") info.encoder = val;
        else if (key == "creation_time") info.creation_time = val;
    }
}

void MetadataExtractor::extract_stream_info(void* fmt_ctx_ptr, MediaInfo& info) {
    auto* fmt_ctx = static_cast<AVFormatContext*>(fmt_ctx_ptr);
    if (!fmt_ctx) return;

    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        auto* stream = fmt_ctx->streams[i];
        if (!stream || !stream->codecpar) continue;

        auto* codec = avcodec_find_decoder(stream->codecpar->codec_id);

        MediaStreamInfo si;
        si.index = static_cast<int>(i);
        si.codec = codec ? codec->name : "unknown";
        si.bit_rate = static_cast<int>(stream->codecpar->bit_rate);

        if (stream->metadata) {
            AVDictionaryEntry* lang = av_dict_get(stream->metadata, "language", nullptr, 0);
            if (lang) si.language = lang->value;
        }

        switch (stream->codecpar->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                si.width = stream->codecpar->width;
                si.height = stream->codecpar->height;
                if (stream->avg_frame_rate.den > 0) {
                    si.frame_rate = static_cast<double>(stream->avg_frame_rate.num) /
                                    stream->avg_frame_rate.den;
                }
                si.codec_profile = "unknown";
                info.video_streams.push_back(si);
                break;

            case AVMEDIA_TYPE_AUDIO:
                si.sample_rate = stream->codecpar->sample_rate;
                si.channels = stream->codecpar->ch_layout.nb_channels;
                {
                    char layout_buf[64] = {};
                    av_channel_layout_describe(&stream->codecpar->ch_layout, layout_buf, sizeof(layout_buf));
                    si.channel_layout = layout_buf;
                }
                info.audio_streams.push_back(si);
                break;

            case AVMEDIA_TYPE_SUBTITLE:
                info.subtitle_streams.push_back(si);
                break;

            default:
                break;
        }
    }
}

} // namespace rav

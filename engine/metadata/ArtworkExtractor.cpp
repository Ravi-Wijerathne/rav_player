#include "ArtworkExtractor.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <cstring>

namespace rav {

bool ArtworkExtractor::has_artwork(const std::string& file_path) {
    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, file_path.c_str(), nullptr, nullptr) != 0) {
        return false;
    }

    bool found = false;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        auto* st = fmt_ctx->streams[i];
        if (st && st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            found = true;
            break;
        }
    }

    if (!found) {
        AVDictionaryEntry* tag = av_dict_get(fmt_ctx->metadata, "comment", nullptr, 0);
        if (tag && strstr(tag->value, "APIC")) {
            found = true;
        }
    }

    avformat_close_input(&fmt_ctx);
    return found;
}

std::vector<uint8_t> ArtworkExtractor::extract_artwork(const std::string& file_path) {
    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, file_path.c_str(), nullptr, nullptr) != 0) {
        return {};
    }
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        return {};
    }

    auto data = extract_apic(fmt_ctx);

    if (data.empty() && fmt_ctx->metadata) {
        AVDictionaryEntry* tag = nullptr;
        while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
            if (strstr(tag->key, "cover") || strstr(tag->key, "picture")) {
                auto val_len = strlen(tag->value);
                data.resize(val_len);
                if (!data.empty()) {
                    std::memcpy(data.data(), tag->value, data.size());
                }
                break;
            }
        }
    }

    avformat_close_input(&fmt_ctx);
    return data;
}

ArtworkFormat ArtworkExtractor::detect_format(const std::vector<uint8_t>& data) {
    if (data.size() < 4) return ArtworkFormat::Unknown;

    if (data[0] == 0xFF && data[1] == 0xD8) return ArtworkFormat::JPEG;
    if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47) {
        return ArtworkFormat::PNG;
    }
    if (data[0] == 'B' && data[1] == 'M') return ArtworkFormat::BMP;
    if (data[0] == 'G' && data[1] == 'I' && data[2] == 'F') return ArtworkFormat::GIF;

    return ArtworkFormat::Unknown;
}

bool ArtworkExtractor::is_valid_image(const std::vector<uint8_t>& data) {
    return detect_format(data) != ArtworkFormat::Unknown;
}

std::vector<uint8_t> ArtworkExtractor::extract_apic(void* fmt_ctx_ptr) {
    auto* fmt_ctx = static_cast<AVFormatContext*>(fmt_ctx_ptr);
    if (!fmt_ctx) return {};

    auto* pkt = av_packet_alloc();
    if (!pkt) return {};

    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        auto* st = fmt_ctx->streams[i];
        if (!st || !(st->disposition & AV_DISPOSITION_ATTACHED_PIC)) continue;

        if (av_read_frame(fmt_ctx, pkt) >= 0) {
            std::vector<uint8_t> data(pkt->data, pkt->data + pkt->size);
            av_packet_free(&pkt);
            return data;
        }
    }

    av_packet_free(&pkt);
    return {};
}

} // namespace rav

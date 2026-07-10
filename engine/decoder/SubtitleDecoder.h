#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "../ffmpeg/FFmpegContext.h"
#include "../subtitles/SubtitleFrame.h"

namespace rav {

class SubtitleDecoder {
public:
    SubtitleDecoder() = default;

    ~SubtitleDecoder() = default;

    SubtitleDecoder(const SubtitleDecoder&) = delete;
    SubtitleDecoder& operator=(const SubtitleDecoder&) = delete;

    SubtitleDecoder(SubtitleDecoder&&) = delete;
    SubtitleDecoder& operator=(SubtitleDecoder&&) = delete;

    bool open(AVCodecParameters* codecpar,
              const AVCodec* codec = nullptr) {
        std::lock_guard<std::mutex> lock(codec_mutex_);
        if (!codecpar) return false;
        if (!codec) {
            codec = avcodec_find_decoder(codecpar->codec_id);
        }
        if (!codec) return false;

        codec_ctx_.reset(avcodec_alloc_context3(codec));
        if (!codec_ctx_) return false;

        int ret = avcodec_parameters_to_context(codec_ctx_.get(), codecpar);
        if (ret < 0) return false;

        ret = avcodec_open2(codec_ctx_.get(), codec, nullptr);
        return ret >= 0;
    }

    void flush() {
        std::lock_guard<std::mutex> lock(codec_mutex_);
        if (codec_ctx_) {
            avcodec_flush_buffers(codec_ctx_.get());
        }
    }

    void close() {
        std::lock_guard<std::mutex> lock(codec_mutex_);
        codec_ctx_.reset();
    }

    bool is_open() const { return codec_ctx_ != nullptr; }

    struct DecodedSubtitle {
        SubtitleFrame frame;
        int64_t pts{AV_NOPTS_VALUE};
        double time_base{0.0};
    };

    // Decode a single subtitle packet.
    // May produce zero, one, or multiple subtitle frames.
    std::vector<DecodedSubtitle> decode(AVPacket* pkt, double stream_time_base) {
        std::vector<DecodedSubtitle> result;
        std::lock_guard<std::mutex> lock(codec_mutex_);
        if (!codec_ctx_) return result;

        AVSubtitle sub;
        std::memset(&sub, 0, sizeof(sub));
        int got_sub = 0;

        int ret = avcodec_decode_subtitle2(codec_ctx_.get(), &sub, &got_sub, pkt);
        if (ret < 0 || !got_sub) {
            avsubtitle_free(&sub);
            return result;
        }

        DecodedSubtitle ds;
        ds.pts = sub.pts;
        ds.time_base = stream_time_base;
        ds.frame.start_time = sub.pts * stream_time_base
                            + sub.start_display_time / 1000.0;
        ds.frame.end_time = sub.pts * stream_time_base
                          + sub.end_display_time / 1000.0;

        for (unsigned i = 0; i < sub.num_rects; ++i) {
            auto* rect = sub.rects[i];
            if (!rect) continue;

            switch (rect->type) {
                case SUBTITLE_TEXT:
                    if (rect->text) {
                        if (!ds.frame.text.empty()) ds.frame.text += '\n';
                        ds.frame.text += rect->text;
                    }
                    break;
                case SUBTITLE_ASS:
                    if (rect->ass) {
                        if (!ds.frame.text.empty()) ds.frame.text += '\n';
                        ds.frame.text += rect->ass;
                    }
                    break;
                case SUBTITLE_BITMAP:
                    if (rect->data[0] && rect->w > 0 && rect->h > 0) {
                        ds.frame.is_bitmap = true;
                        size_t bmp_size = rect->w * rect->h * 4;
                        ds.frame.bitmap_data.resize(bmp_size);
                        std::memcpy(ds.frame.bitmap_data.data(),
                                    rect->data[0], bmp_size);
                        ds.frame.width = rect->w;
                        ds.frame.height = rect->h;
                        ds.frame.x = rect->x;
                        ds.frame.y = rect->y;
                    }
                    break;
                default:
                    break;
            }
        }

        result.push_back(std::move(ds));
        avsubtitle_free(&sub);
        return result;
    }

    AVCodecContext* context() { return codec_ctx_.get(); }

private:
    CodecContextPtr codec_ctx_;
    std::mutex codec_mutex_;
};

} // namespace rav

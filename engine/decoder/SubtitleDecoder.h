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

    static std::string extract_ass_text(const char* ass_line) {
        std::string line(ass_line);
        
        int commas = 0;
        size_t pos = 0;
        
        if (line.find("Dialogue:") == 0) {
            while (commas < 9 && pos < line.size()) {
                if (line[pos] == ',') commas++;
                pos++;
            }
        } else {
            while (commas < 8 && pos < line.size()) {
                if (line[pos] == ',') commas++;
                pos++;
            }
        }
        
        if (pos < line.size()) {
            line = line.substr(pos);
        }
        
        std::string result;
        result.reserve(line.size());
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '{') {
                while (i < line.size() && line[i] != '}') ++i;
                continue;
            }
            if (line[i] == '\\' && i + 1 < line.size()) {
                if (line[i + 1] == 'N' || line[i + 1] == 'n') {
                    result += '\n';
                    ++i;
                    continue;
                }
                if (line[i + 1] == 'h') {
                    result += ' ';
                    ++i;
                    continue;
                }
            }
            result += line[i];
        }
        return result;
    }

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
        
        double packet_pts_sec = (pkt->pts == AV_NOPTS_VALUE) ? 0.0 : pkt->pts * stream_time_base;
        double packet_duration_sec = pkt->duration * stream_time_base;
        
        double base_time = packet_pts_sec;
        if (sub.pts != AV_NOPTS_VALUE) {
            base_time = sub.pts / (double)AV_TIME_BASE;
        }
        
        ds.pts = sub.pts;
        ds.time_base = stream_time_base;
        ds.frame.start_time = base_time + sub.start_display_time / 1000.0;
        
        double duration = (sub.end_display_time - sub.start_display_time) / 1000.0;
        if (duration <= 0.0) {
            duration = packet_duration_sec;
        }
        ds.frame.end_time = ds.frame.start_time + duration;

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
                        ds.frame.text += extract_ass_text(rect->ass);
                    }
                    break;
                case SUBTITLE_BITMAP:
                    if (rect->data[0] && rect->w > 0 && rect->h > 0) {
                        ds.frame.is_bitmap = true;
                        size_t bmp_size = rect->w * rect->h * 4;
                        ds.frame.bitmap_data.resize(bmp_size);
                        
                        if (rect->nb_colors > 0 && rect->data[1]) {
                            const uint32_t* pal = reinterpret_cast<const uint32_t*>(rect->data[1]);
                            const uint8_t* src = rect->data[0];
                            int src_linesize = rect->linesize[0];
                            
                            for (int y = 0; y < rect->h; ++y) {
                                for (int x = 0; x < rect->w; ++x) {
                                    uint8_t index = src[y * src_linesize + x];
                                    uint32_t color = pal[index];
                                    
                                    // FFmpeg palette is ARGB in memory (A in highest byte)
                                    uint8_t a = (color >> 24) & 0xFF;
                                    uint8_t r = (color >> 16) & 0xFF;
                                    uint8_t g = (color >> 8) & 0xFF;
                                    uint8_t b = (color >> 0) & 0xFF;
                                    
                                    // CoreGraphics requires premultiplied alpha for RGBA
                                    r = (r * a) / 255;
                                    g = (g * a) / 255;
                                    b = (b * a) / 255;
                                    
                                    // Store as RGBA byte array for easy creation of CGImage/NSImage
                                    size_t out_offset = (y * rect->w + x) * 4;
                                    ds.frame.bitmap_data[out_offset + 0] = r;
                                    ds.frame.bitmap_data[out_offset + 1] = g;
                                    ds.frame.bitmap_data[out_offset + 2] = b;
                                    ds.frame.bitmap_data[out_offset + 3] = a;
                                }
                            }
                        } else {
                            // Fallback if no palette (unlikely for subtitles, but safe to handle)
                            std::memset(ds.frame.bitmap_data.data(), 0, bmp_size);
                        }
                        
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

#pragma once
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "../ffmpeg/FFmpegContext.h"

namespace rav {

class AudioDecoder {
public:
    AudioDecoder() = default;
    ~AudioDecoder() = default;

    AudioDecoder(const AudioDecoder&) = delete;
    AudioDecoder& operator=(const AudioDecoder&) = delete;

    AudioDecoder(AudioDecoder&&) = delete;
    AudioDecoder& operator=(AudioDecoder&&) = delete;

    bool open(AVCodecParameters* codecpar, const AVCodec* codec = nullptr);
    void flush();
    void close();
    bool is_open() const;
    bool send_packet(AVPacket* pkt);
    FramePtr receive_frame();
    
    AVCodecContext* context();
    int sample_rate() const;
    AVSampleFormat sample_format() const;

    CodecContextPtr codec_ctx_;
    std::mutex codec_mutex_;
};

} // namespace rav


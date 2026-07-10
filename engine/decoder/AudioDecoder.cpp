#include "AudioDecoder.h"

namespace rav {

bool AudioDecoder::open(AVCodecParameters* codecpar, const AVCodec* codec) {
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

void AudioDecoder::flush() {
    std::lock_guard<std::mutex> lock(codec_mutex_);
    if (codec_ctx_) {
        avcodec_flush_buffers(codec_ctx_.get());
    }
}

void AudioDecoder::close() { 
    std::lock_guard<std::mutex> lock(codec_mutex_);
    codec_ctx_.reset(); 
}

bool AudioDecoder::is_open() const { 
    return codec_ctx_ != nullptr; 
}

bool AudioDecoder::send_packet(AVPacket* pkt) {
    std::lock_guard<std::mutex> lock(codec_mutex_);
    if (!codec_ctx_) return false;
    return avcodec_send_packet(codec_ctx_.get(), pkt) >= 0;
}

FramePtr AudioDecoder::receive_frame() {
    std::lock_guard<std::mutex> lock(codec_mutex_);
    if (!codec_ctx_) return nullptr;
    auto frame = FramePtr(av_frame_alloc());
    if (!frame) return nullptr;

    int ret = avcodec_receive_frame(codec_ctx_.get(), frame.get());
    if (ret < 0) return nullptr;

    return frame;
}

AVCodecContext* AudioDecoder::context() { 
    return codec_ctx_.get(); 
}

int AudioDecoder::sample_rate() const {
    return codec_ctx_ ? codec_ctx_->sample_rate : 0;
}

AVSampleFormat AudioDecoder::sample_format() const {
    return codec_ctx_ ? codec_ctx_->sample_fmt : AV_SAMPLE_FMT_NONE;
}

} // namespace rav

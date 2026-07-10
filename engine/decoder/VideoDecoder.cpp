#include "VideoDecoder.h"

namespace rav {

bool VideoDecoder::open(AVCodecParameters* codecpar, const AVCodec* codec) {
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

    codec_ctx_->thread_count = 0;
    codec_ctx_->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    if (hw_decoder_) {
        hw_decoder_->init(codec_ctx_.get());
    }

    ret = avcodec_open2(codec_ctx_.get(), codec, nullptr);
    return ret >= 0;
}

void VideoDecoder::flush() {
    std::lock_guard<std::mutex> lock(codec_mutex_);
    if (codec_ctx_) {
        avcodec_flush_buffers(codec_ctx_.get());
    }
}

void VideoDecoder::close() {
    std::lock_guard<std::mutex> lock(codec_mutex_);
    codec_ctx_.reset();
    hw_decoder_.reset();
}

bool VideoDecoder::is_open() const { 
    return codec_ctx_ != nullptr; 
}

bool VideoDecoder::send_packet(AVPacket* pkt) {
    std::lock_guard<std::mutex> lock(codec_mutex_);
    if (!codec_ctx_) return false;
    int ret = avcodec_send_packet(codec_ctx_.get(), pkt);
    return ret >= 0;
}

FramePtr VideoDecoder::receive_frame() {
    std::lock_guard<std::mutex> lock(codec_mutex_);
    if (!codec_ctx_) return nullptr;
    auto frame = FramePtr(av_frame_alloc());
    if (!frame) return nullptr;

    int ret = avcodec_receive_frame(codec_ctx_.get(), frame.get());
    if (ret < 0) return nullptr;

    return frame;
}

void VideoDecoder::set_hardware_decoder(std::unique_ptr<HardwareDecoder> hw) {
    hw_decoder_ = std::move(hw);
}

AVCodecContext* VideoDecoder::context() { 
    return codec_ctx_.get(); 
}

int VideoDecoder::width() const {
    return codec_ctx_ ? codec_ctx_->width : 0;
}

int VideoDecoder::height() const {
    return codec_ctx_ ? codec_ctx_->height : 0;
}

AVPixelFormat VideoDecoder::pixel_format() const {
    return codec_ctx_ ? codec_ctx_->pix_fmt : AV_PIX_FMT_NONE;
}

} // namespace rav

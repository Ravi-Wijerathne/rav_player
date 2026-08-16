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

    AVBufferRef* hw_device_ctx = nullptr;
    int hw_ret = av_hwdevice_ctx_create(
        &hw_device_ctx, AV_HWDEVICE_TYPE_VIDEOTOOLBOX, nullptr, nullptr, 0);
    if (hw_ret >= 0) {
        codec_ctx_->hw_device_ctx = hw_device_ctx;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(60, 0, 0)
        codec_ctx_->hw_pix_fmt = AV_PIX_FMT_VIDEOTOOLBOX;
#endif
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

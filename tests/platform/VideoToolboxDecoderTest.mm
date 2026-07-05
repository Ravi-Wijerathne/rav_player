#include <gtest/gtest.h>

#include "platform/VideoToolboxDecoder.h"

using namespace rav;

TEST(VideoToolboxDecoderTest, CreateAndDestroy) {
    VideoToolboxDecoder decoder;
    SUCCEED();
}

TEST(VideoToolboxDecoderTest, TypeReturnsVideoToolbox) {
    VideoToolboxDecoder decoder;
    EXPECT_EQ(decoder.type(), HardwareDecoderType::VideoToolbox);
}

TEST(VideoToolboxDecoderTest, HwDeviceTypeReturnsVideoToolbox) {
    VideoToolboxDecoder decoder;
    EXPECT_EQ(decoder.hw_device_type(), AV_HWDEVICE_TYPE_VIDEOTOOLBOX);
}

TEST(VideoToolboxDecoderTest, IsAvailableReturnsTrue) {
    VideoToolboxDecoder decoder;
    EXPECT_TRUE(decoder.is_available());
}

TEST(VideoToolboxDecoderTest, InitWithNullContextReturnsFalse) {
    VideoToolboxDecoder decoder;
    EXPECT_FALSE(decoder.init(nullptr));
}

TEST(VideoToolboxDecoderTest, InitWithValidContextSucceeds) {
    VideoToolboxDecoder decoder;
    AVCodecContext* codec_ctx = avcodec_alloc_context3(nullptr);
    ASSERT_NE(codec_ctx, nullptr);
    bool result = decoder.init(codec_ctx);
    EXPECT_TRUE(result);
    EXPECT_NE(codec_ctx->hw_device_ctx, nullptr);
    avcodec_free_context(&codec_ctx);
}

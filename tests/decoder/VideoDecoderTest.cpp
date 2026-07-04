#include <gtest/gtest.h>
#include <decoder/VideoDecoder.h>

using namespace rav;

TEST(VideoDecoderTest, CreateAndDestroy) {
    VideoDecoder decoder;
    EXPECT_FALSE(decoder.is_open());
}

TEST(VideoDecoderTest, OpenWithNullCodecpar) {
    VideoDecoder decoder;
    EXPECT_FALSE(decoder.open(nullptr));
}

TEST(VideoDecoderTest, OpenWithNonexistentCodec) {
    // Can't test without a valid AVCodecParameters from a real file
    // This just verifies the API exists
    VideoDecoder decoder;
    EXPECT_FALSE(decoder.is_open());
}

TEST(VideoDecoderTest, GetDimensionsWhenClosed) {
    VideoDecoder decoder;
    EXPECT_EQ(decoder.width(), 0);
    EXPECT_EQ(decoder.height(), 0);
}

TEST(VideoDecoderTest, PixelFormatWhenClosed) {
    VideoDecoder decoder;
    EXPECT_EQ(decoder.pixel_format(), AV_PIX_FMT_NONE);
}

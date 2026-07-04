#include <gtest/gtest.h>
#include <decoder/AudioDecoder.h>

using namespace rav;

TEST(AudioDecoderTest, CreateAndDestroy) {
    AudioDecoder decoder;
    EXPECT_FALSE(decoder.is_open());
}

TEST(AudioDecoderTest, OpenWithNullCodecpar) {
    AudioDecoder decoder;
    EXPECT_FALSE(decoder.open(nullptr));
}

TEST(AudioDecoderTest, SampleRateWhenClosed) {
    AudioDecoder decoder;
    EXPECT_EQ(decoder.sample_rate(), 0);
}

TEST(AudioDecoderTest, SampleFormatWhenClosed) {
    AudioDecoder decoder;
    EXPECT_EQ(decoder.sample_format(), AV_SAMPLE_FMT_NONE);
}

TEST(AudioDecoderTest, CloseWhenNotOpen) {
    AudioDecoder decoder;
    EXPECT_NO_THROW(decoder.close());
}

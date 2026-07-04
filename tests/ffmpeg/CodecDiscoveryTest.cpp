#include <gtest/gtest.h>
#include <ffmpeg/CodecDiscovery.h>

using namespace rav;

TEST(CodecDiscoveryTest, FindH264Decoder) {
    auto* codec = CodecDiscovery::find_decoder(AV_CODEC_ID_H264);
    ASSERT_NE(codec, nullptr);
    EXPECT_STREQ(codec->name, "h264");
}

TEST(CodecDiscoveryTest, FindAACDecoder) {
    auto* codec = CodecDiscovery::find_decoder(AV_CODEC_ID_AAC);
    ASSERT_NE(codec, nullptr);
    EXPECT_EQ(codec->type, AVMEDIA_TYPE_AUDIO);
}

TEST(CodecDiscoveryTest, FindNonexistentDecoder) {
    auto* codec = CodecDiscovery::find_decoder(AV_CODEC_ID_NONE);
    EXPECT_EQ(codec, nullptr);
}

TEST(CodecDiscoveryTest, DescribeNullCodec) {
    auto info = CodecDiscovery::describe_codec(nullptr);
    EXPECT_FALSE(info.has_value());
}

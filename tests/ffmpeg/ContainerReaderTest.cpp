#include <gtest/gtest.h>
#include <ffmpeg/ContainerReader.h>
#include <ffmpeg/CodecDiscovery.h>
#include <ffmpeg/PacketGenerator.h>

using namespace rav;

TEST(ContainerReaderTest, OpenNonexistentFile) {
    ContainerReader reader;
    EXPECT_FALSE(reader.open("/nonexistent/file.mp4"));
    EXPECT_FALSE(reader.is_open());
}

TEST(ContainerReaderTest, DefaultInfo) {
    ContainerReader reader;
    const auto& info = reader.info();
    EXPECT_TRUE(info.filename.empty());
    EXPECT_EQ(info.duration, 0);
}

TEST(ContainerReaderTest, CodecDiscoveryFindDecoder) {
    auto* codec = CodecDiscovery::find_decoder(AV_CODEC_ID_H264);
    ASSERT_NE(codec, nullptr);
    EXPECT_NE(codec->id, AV_CODEC_ID_NONE);
}

TEST(ContainerReaderTest, CodecDiscoveryFindDecoderByName) {
    auto* codec = CodecDiscovery::find_decoder_by_name("h264");
    ASSERT_NE(codec, nullptr);
    EXPECT_EQ(codec->id, AV_CODEC_ID_H264);
}

TEST(ContainerReaderTest, CodecDiscoveryDescribeCodec) {
    auto* codec = CodecDiscovery::find_decoder(AV_CODEC_ID_AAC);
    ASSERT_NE(codec, nullptr);

    auto info = CodecDiscovery::describe_codec(codec);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->id, AV_CODEC_ID_AAC);
    EXPECT_EQ(info->media_type, AVMEDIA_TYPE_AUDIO);
}

TEST(ContainerReaderTest, PacketGeneratorFromReader) {
    ContainerReader reader;
    PacketGenerator gen(reader);
    auto pkt = gen.next();
    EXPECT_EQ(pkt, nullptr);
}

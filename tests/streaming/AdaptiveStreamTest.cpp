#include <gtest/gtest.h>
#include <streaming/AdaptiveStream.h>

using namespace rav;

TEST(AdaptiveStreamTest, DefaultConfig) {
    AdaptiveStream stream;
    EXPECT_EQ(stream.current_bitrate(), 500000);
    EXPECT_EQ(stream.variant_count(), 0);
}

TEST(AdaptiveStreamTest, AddVariants) {
    AdaptiveStream stream;
    stream.add_variant(500000, 640, 360);
    stream.add_variant(1000000, 1280, 720);
    stream.add_variant(2000000, 1920, 1080);
    EXPECT_EQ(stream.variant_count(), 3);
}

TEST(AdaptiveStreamTest, SelectLowQuality) {
    AdaptiveStream stream;
    stream.add_variant(500000, 640, 360);
    stream.add_variant(1000000, 1280, 720);
    stream.add_variant(2000000, 1920, 1080);
    stream.set_quality(AdaptiveStreamQuality::Low);
    auto bitrate = stream.select_bitrate();
    EXPECT_EQ(bitrate, 500000);
}

TEST(AdaptiveStreamTest, SelectHighQuality) {
    AdaptiveStream stream;
    stream.add_variant(500000, 640, 360);
    stream.add_variant(1000000, 1280, 720);
    stream.add_variant(2000000, 1920, 1080);
    stream.set_quality(AdaptiveStreamQuality::High);
    auto bitrate = stream.select_bitrate();
    EXPECT_EQ(bitrate, 2000000);
}

TEST(AdaptiveStreamTest, QualityGetters) {
    AdaptiveStream stream;
    EXPECT_EQ(stream.quality(), AdaptiveStreamQuality::Auto);
    stream.set_quality(AdaptiveStreamQuality::Medium);
    EXPECT_EQ(stream.quality(), AdaptiveStreamQuality::Medium);
}

TEST(AdaptiveStreamTest, Reset) {
    AdaptiveStream stream;
    stream.add_variant(500000, 640, 360);
    stream.add_variant(1000000, 1280, 720);
    stream.select_bitrate();
    stream.reset();
    EXPECT_EQ(stream.current_bitrate(), 500000);
}

TEST(AdaptiveStreamTest, ReportDownloadTime) {
    AdaptiveStream stream;
    stream.add_variant(500000, 640, 360);
    stream.add_variant(1000000, 1280, 720);
    stream.add_variant(2000000, 1920, 1080);
    stream.report_download_time(100000, std::chrono::milliseconds(100));
    auto bitrate = stream.select_bitrate();
    EXPECT_GT(bitrate, 0);
}

TEST(AdaptiveStreamTest, CustomConfig) {
    AdaptiveStreamConfig config;
    config.initial_bitrate = 1000000;
    config.min_bitrate = 50000;
    config.max_bitrate = 5000000;
    AdaptiveStream stream(config);
    EXPECT_EQ(stream.current_bitrate(), 1000000);
}

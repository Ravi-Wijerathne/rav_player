#include <gtest/gtest.h>
#include <media/StreamSource.h>

using namespace rav;

TEST(StreamSourceTest, HttpSource) {
    StreamSource src("http://example.com/stream", MediaSourceType::HTTP);
    EXPECT_EQ(src.type(), MediaSourceType::HTTP);
    EXPECT_TRUE(src.open());
    EXPECT_TRUE(src.is_open());
    EXPECT_EQ(src.url(), "http://example.com/stream");
}

TEST(StreamSourceTest, HttpsSource) {
    StreamSource src("https://example.com/stream", MediaSourceType::HTTPS);
    EXPECT_EQ(src.type(), MediaSourceType::HTTPS);
    EXPECT_TRUE(src.open());
}

TEST(StreamSourceTest, DetectType) {
    EXPECT_EQ(StreamSource::detect_type("http://example.com"), MediaSourceType::HTTP);
    EXPECT_EQ(StreamSource::detect_type("https://example.com"), MediaSourceType::HTTPS);
    EXPECT_EQ(StreamSource::detect_type("rtsp://example.com"), MediaSourceType::RTSP);
    EXPECT_EQ(StreamSource::detect_type("file.mp4"), MediaSourceType::Unknown);
}

TEST(StreamSourceTest, Close) {
    StreamSource src("http://example.com", MediaSourceType::HTTP);
    src.open();
    src.close();
    EXPECT_FALSE(src.is_open());
}

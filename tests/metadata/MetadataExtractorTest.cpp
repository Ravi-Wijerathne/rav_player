#include <gtest/gtest.h>
#include <metadata/MetadataExtractor.h>

using namespace rav;

TEST(MetadataExtractorTest, NonExistentFile) {
    MetadataExtractor extractor;
    auto info = extractor.extract("/nonexistent/file.mp4");
    EXPECT_TRUE(info.format_name.empty());
    EXPECT_EQ(info.duration.count(), 0);
}

TEST(MetadataExtractorTest, MediaInfoDefaults) {
    MediaInfo info;
    EXPECT_FALSE(info.has_video());
    EXPECT_FALSE(info.has_audio());
    EXPECT_FALSE(info.has_subtitles());
    EXPECT_TRUE(info.video_streams.empty());
    EXPECT_TRUE(info.audio_streams.empty());
    EXPECT_TRUE(info.subtitle_streams.empty());
    EXPECT_EQ(info.duration.count(), 0);
}

TEST(MetadataExtractorTest, MediaStreamInfoDefaults) {
    MediaStreamInfo si;
    EXPECT_EQ(si.index, -1);
    EXPECT_EQ(si.width, 0);
    EXPECT_EQ(si.height, 0);
    EXPECT_EQ(si.sample_rate, 0);
    EXPECT_EQ(si.channels, 0);
    EXPECT_EQ(si.bit_rate, 0);
}

TEST(MetadataExtractorTest, HasEmbeddedMetadata) {
    MetadataExtractor extractor;
    EXPECT_FALSE(extractor.has_embedded_metadata("/nonexistent/file.mp4"));
}

TEST(MetadataExtractorTest, ExtractNonExistentURI) {
    MetadataExtractor extractor;
    auto info = extractor.extract("http://nonexistent.example.com/stream", {});
    EXPECT_TRUE(info.format_name.empty());
}

#include <gtest/gtest.h>
#include <metadata/ArtworkExtractor.h>

using namespace rav;

TEST(ArtworkExtractorTest, NoArtworkForNonexistentFile) {
    ArtworkExtractor extractor;
    EXPECT_FALSE(extractor.has_artwork("/nonexistent/file.mp4"));
}

TEST(ArtworkExtractorTest, ExtractFromNonexistentFile) {
    ArtworkExtractor extractor;
    auto data = extractor.extract_artwork("/nonexistent/file.mp4");
    EXPECT_TRUE(data.empty());
}

TEST(ArtworkExtractorTest, DetectJPEG) {
    std::vector<uint8_t> jpeg = {0xFF, 0xD8, 0xFF, 0xE0, 0x00};
    ArtworkExtractor extractor;
    EXPECT_EQ(extractor.detect_format(jpeg), ArtworkFormat::JPEG);
}

TEST(ArtworkExtractorTest, DetectPNG) {
    std::vector<uint8_t> png = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A};
    ArtworkExtractor extractor;
    EXPECT_EQ(extractor.detect_format(png), ArtworkFormat::PNG);
}

TEST(ArtworkExtractorTest, DetectBMP) {
    std::vector<uint8_t> bmp = {'B', 'M', 0x00, 0x00};
    ArtworkExtractor extractor;
    EXPECT_EQ(extractor.detect_format(bmp), ArtworkFormat::BMP);
}

TEST(ArtworkExtractorTest, DetectGIF) {
    std::vector<uint8_t> gif = {'G', 'I', 'F', '8', '9', 'a'};
    ArtworkExtractor extractor;
    EXPECT_EQ(extractor.detect_format(gif), ArtworkFormat::GIF);
}

TEST(ArtworkExtractorTest, DetectUnknown) {
    std::vector<uint8_t> unknown = {0x00, 0x01, 0x02, 0x03};
    ArtworkExtractor extractor;
    EXPECT_EQ(extractor.detect_format(unknown), ArtworkFormat::Unknown);
}

TEST(ArtworkExtractorTest, DetectEmptyData) {
    ArtworkExtractor extractor;
    EXPECT_EQ(extractor.detect_format({}), ArtworkFormat::Unknown);
}

TEST(ArtworkExtractorTest, IsValidImage) {
    ArtworkExtractor extractor;
    EXPECT_TRUE(extractor.is_valid_image({0xFF, 0xD8, 0xFF, 0xE0}));
    EXPECT_FALSE(extractor.is_valid_image({0x00, 0x01, 0x02}));
    EXPECT_FALSE(extractor.is_valid_image({}));
}

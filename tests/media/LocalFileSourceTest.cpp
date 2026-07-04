#include <gtest/gtest.h>
#include <media/LocalFileSource.h>

using namespace rav;

TEST(LocalFileSourceTest, OpenNonexistentFile) {
    LocalFileSource src("/nonexistent/file.mp4");
    EXPECT_FALSE(src.open());
    EXPECT_FALSE(src.is_open());
}

TEST(LocalFileSourceTest, TypeIsLocalFile) {
    LocalFileSource src("/tmp/test.mp4");
    EXPECT_EQ(src.type(), MediaSourceType::LocalFile);
}

TEST(LocalFileSourceTest, UrlReturnsPath) {
    LocalFileSource src("/path/to/video.mp4");
    EXPECT_EQ(src.url(), "/path/to/video.mp4");
}

TEST(LocalFileSourceTest, Close) {
    LocalFileSource src("/tmp/test.mp4");
    src.open();
    src.close();
    EXPECT_FALSE(src.is_open());
}

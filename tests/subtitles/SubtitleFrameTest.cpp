#include <gtest/gtest.h>
#include <subtitles/SubtitleFrame.h>

using namespace rav;

TEST(SubtitleFrameTest, DefaultFrame) {
    SubtitleFrame f;
    EXPECT_DOUBLE_EQ(f.start_time, 0.0);
    EXPECT_DOUBLE_EQ(f.end_time, 0.0);
    EXPECT_TRUE(f.text.empty());
    EXPECT_FALSE(f.is_bitmap);
    EXPECT_FALSE(f.is_valid());
}

TEST(SubtitleFrameTest, TextFrameIsValid) {
    SubtitleFrame f;
    f.text = "Hello";
    EXPECT_TRUE(f.is_valid());
}

TEST(SubtitleFrameTest, BitmapFrameIsValid) {
    SubtitleFrame f;
    f.is_bitmap = true;
    f.bitmap_data.resize(100);
    EXPECT_TRUE(f.is_valid());
}

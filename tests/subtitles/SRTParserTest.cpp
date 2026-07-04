#include <gtest/gtest.h>
#include <subtitles/SRTParser.h>

using namespace rav;

static const char* TEST_SRT = R"(1
00:00:01,000 --> 00:00:04,000
Hello world

2
00:00:05,500 --> 00:00:08,000
This is a test
Second line

3
00:01:00,000 --> 00:01:30,000
Long subtitle
)";

TEST(SRTParserTest, Format) {
    SRTParser parser;
    EXPECT_EQ(parser.format(), SubtitleFormat::SRT);
}

TEST(SRTParserTest, ParseValidSRT) {
    SRTParser parser;
    ASSERT_TRUE(parser.load(TEST_SRT));
    auto frames = parser.parse();
    ASSERT_EQ(frames.size(), 3);
}

TEST(SRTParserTest, FirstFrameTiming) {
    SRTParser parser;
    parser.load(TEST_SRT);
    auto frames = parser.parse();

    EXPECT_DOUBLE_EQ(frames[0].start_time, 1.0);
    EXPECT_DOUBLE_EQ(frames[0].end_time, 4.0);
}

TEST(SRTParserTest, FirstFrameText) {
    SRTParser parser;
    parser.load(TEST_SRT);
    auto frames = parser.parse();
    EXPECT_EQ(frames[0].text, "Hello world");
}

TEST(SRTParserTest, MultilineText) {
    SRTParser parser;
    parser.load(TEST_SRT);
    auto frames = parser.parse();
    EXPECT_EQ(frames[1].text, "This is a test\nSecond line");
}

TEST(SRTParserTest, LongSubtitleTiming) {
    SRTParser parser;
    parser.load(TEST_SRT);
    auto frames = parser.parse();
    EXPECT_DOUBLE_EQ(frames[2].start_time, 60.0);
    EXPECT_DOUBLE_EQ(frames[2].end_time, 90.0);
}

TEST(SRTParserTest, FramesAtTime) {
    SRTParser parser;
    parser.load(TEST_SRT);
    parser.parse();

    auto at_time_2 = parser.frames_at_time(2.0);
    ASSERT_EQ(at_time_2.size(), 1);
    EXPECT_EQ(at_time_2[0].text, "Hello world");

    auto at_time_5 = parser.frames_at_time(5.0);
    EXPECT_TRUE(at_time_5.empty());
}

TEST(SRTParserTest, LoadNonexistentFile) {
    SRTParser parser;
    EXPECT_FALSE(parser.load_file("/nonexistent/file.srt"));
}

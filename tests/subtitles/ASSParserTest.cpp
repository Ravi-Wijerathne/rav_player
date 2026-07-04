#include <gtest/gtest.h>
#include <subtitles/ASSParser.h>

using namespace rav;

static const char* TEST_ASS = R"([Script Info]
Title: Test

[Events]
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
Dialogue: 0,0:00:01.00,0:00:04.00,Default,,0,0,0,,Hello world
Dialogue: 0,0:00:05.50,0:00:08.00,Default,,0,0,0,,This is a {\b1}bold{\b0} test
Dialogue: 0,0:01:00.00,0:01:30.00,Default,,0,0,0,,Line 1\nLine 2
)";

TEST(ASSParserTest, Format) {
    ASSParser parser;
    EXPECT_EQ(parser.format(), SubtitleFormat::ASS);
}

TEST(ASSParserTest, ParseValidASS) {
    ASSParser parser;
    ASSERT_TRUE(parser.load(TEST_ASS));
    auto frames = parser.parse();
    ASSERT_EQ(frames.size(), 3);
}

TEST(ASSParserTest, FirstFrameTiming) {
    ASSParser parser;
    parser.load(TEST_ASS);
    auto frames = parser.parse();
    EXPECT_NEAR(frames[0].start_time, 1.0, 0.01);
    EXPECT_NEAR(frames[0].end_time, 4.0, 0.01);
}

TEST(ASSParserTest, FirstFrameText) {
    ASSParser parser;
    parser.load(TEST_ASS);
    auto frames = parser.parse();
    EXPECT_EQ(frames[0].text, "Hello world");
}

TEST(ASSParserTest, StripsOverrides) {
    ASSParser parser;
    parser.load(TEST_ASS);
    auto frames = parser.parse();
    EXPECT_EQ(frames[1].text, "This is a bold test");
}

TEST(ASSParserTest, HandlesNewlines) {
    ASSParser parser;
    parser.load(TEST_ASS);
    auto frames = parser.parse();
    EXPECT_EQ(frames[2].text, "Line 1\nLine 2");
}

TEST(ASSParserTest, LoadNonexistentFile) {
    ASSParser parser;
    EXPECT_FALSE(parser.load_file("/nonexistent/file.ass"));
}

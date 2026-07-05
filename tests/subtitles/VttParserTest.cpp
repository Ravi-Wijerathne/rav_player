#include <gtest/gtest.h>
#include "subtitles/VttParser.h"

using namespace rav;

TEST(VttParserTest, Format) {
    VttParser parser;
    EXPECT_EQ(parser.format(), SubtitleFormat::WebVTT);
}

TEST(VttParserTest, ParseValidVTT) {
    std::string vtt_data = R"(WEBVTT

00:00:01.000 --> 00:00:04.000
Hello World

00:00:05.000 --> 00:00:08.500
Second subtitle)";

    VttParser parser;
    ASSERT_TRUE(parser.load(vtt_data));
    auto frames = parser.parse();
    ASSERT_EQ(frames.size(), 2u);
    EXPECT_EQ(frames[0].text, "Hello World");
    EXPECT_DOUBLE_EQ(frames[0].start_time, 1.0);
    EXPECT_DOUBLE_EQ(frames[0].end_time, 4.0);
    EXPECT_DOUBLE_EQ(frames[1].start_time, 5.0);
    EXPECT_DOUBLE_EQ(frames[1].end_time, 8.5);
}

TEST(VttParserTest, MultilineText) {
    std::string vtt_data = R"(WEBVTT

00:00:01.000 --> 00:00:03.000
Line one
Line two)";

    VttParser parser;
    parser.load(vtt_data);
    auto frames = parser.parse();
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].text, "Line one\nLine two");
}

TEST(VttParserTest, LoadNonexistentFile) {
    VttParser parser;
    EXPECT_FALSE(parser.load_file("/nonexistent/file.vtt"));
}

TEST(VttParserTest, ParseEmptyContent) {
    VttParser parser;
    parser.load("");
    auto frames = parser.parse();
    EXPECT_TRUE(frames.empty());
}

TEST(VttParserTest, FramesAtTime) {
    std::string vtt_data = R"(WEBVTT

00:00:01.000 --> 00:00:04.000
Hello World

00:00:05.000 --> 00:00:08.500
Second subtitle)";

    VttParser parser;
    parser.load(vtt_data);
    parser.parse();

    auto at_time_2 = parser.frames_at_time(2.0);
    ASSERT_EQ(at_time_2.size(), 1u);
    EXPECT_EQ(at_time_2[0].text, "Hello World");

    auto at_time_10 = parser.frames_at_time(10.0);
    EXPECT_TRUE(at_time_10.empty());
}

TEST(VttParserTest, MMSSFormat) {
    std::string vtt_data = R"(WEBVTT

01:23.500 --> 02:45.000
Short timestamp)";

    VttParser parser;
    parser.load(vtt_data);
    auto frames = parser.parse();
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_DOUBLE_EQ(frames[0].start_time, 83.5);
    EXPECT_DOUBLE_EQ(frames[0].end_time, 165.0);
}

#include <gtest/gtest.h>
#include "subtitles/SubtitleQueue.h"
#include "subtitles/SubtitleFrame.h"

using namespace rav;

TEST(SubtitleQueueTest, InitiallyEmpty) {
    SubtitleQueue queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0u);
}

TEST(SubtitleQueueTest, PushAndQuery) {
    SubtitleQueue queue;
    SubtitleFrame frame;
    frame.start_time = 1.0;
    frame.end_time = 5.0;
    frame.text = "Hello";
    queue.push(std::move(frame));

    EXPECT_EQ(queue.size(), 1u);

    auto result = queue.subtitles_at_time(3.0);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].text, "Hello");
}

TEST(SubtitleQueueTest, NoSubtitleAtTime) {
    SubtitleQueue queue;
    SubtitleFrame frame;
    frame.start_time = 1.0;
    frame.end_time = 2.0;
    frame.text = "Hi";
    queue.push(std::move(frame));

    auto result = queue.subtitles_at_time(5.0);
    EXPECT_TRUE(result.empty());
}

TEST(SubtitleQueueTest, ExpiredSubtitleIsRemoved) {
    SubtitleQueue queue;
    SubtitleFrame frame;
    frame.start_time = 1.0;
    frame.end_time = 2.0;
    frame.text = "Gone";
    queue.push(std::move(frame));

    auto result = queue.subtitles_at_time(3.0);
    EXPECT_TRUE(result.empty());
    EXPECT_TRUE(queue.empty());
}

TEST(SubtitleQueueTest, Clear) {
    SubtitleQueue queue;
    SubtitleFrame frame;
    frame.text = "Clear me";
    queue.push(std::move(frame));
    queue.clear();
    EXPECT_TRUE(queue.empty());
}

TEST(SubtitleQueueTest, MultipleFramesOrdered) {
    SubtitleQueue queue;
    SubtitleFrame f1; f1.start_time = 1.0; f1.end_time = 2.0; f1.text = "First";
    SubtitleFrame f2; f2.start_time = 2.5; f2.end_time = 4.0; f2.text = "Second";
    queue.push(std::move(f1));
    queue.push(std::move(f2));

    auto r1 = queue.subtitles_at_time(1.5);
    ASSERT_EQ(r1.size(), 1u);
    EXPECT_EQ(r1[0].text, "First");

    auto r2 = queue.subtitles_at_time(3.0);
    ASSERT_EQ(r2.size(), 1u);
    EXPECT_EQ(r2[0].text, "Second");
}

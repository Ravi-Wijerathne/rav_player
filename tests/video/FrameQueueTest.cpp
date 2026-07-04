#include <gtest/gtest.h>
#include <video/FrameQueue.h>
#include <thread>

using namespace rav;

static VideoFrame make_dummy_frame() {
    VideoFrame f;
    f.width = 1920;
    f.height = 1080;
    f.pts = 0.0;
    return f;
}

TEST(FrameQueueTest, InitiallyEmpty) {
    FrameQueue q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0);
}

TEST(FrameQueueTest, PushAndSize) {
    FrameQueue q;
    q.push(make_dummy_frame());
    EXPECT_FALSE(q.empty());
    EXPECT_EQ(q.size(), 1);
}

TEST(FrameQueueTest, PushAndPop) {
    FrameQueue q;
    auto f = make_dummy_frame();
    f.pts = 42.0;
    q.push(std::move(f));

    auto popped = q.pop();
    EXPECT_EQ(popped.pts, 42.0);
    EXPECT_TRUE(q.empty());
}

TEST(FrameQueueTest, PopBlocksUntilPush) {
    FrameQueue q;
    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        q.push(make_dummy_frame());
    });

    auto f = q.pop();
    EXPECT_EQ(f.width, 1920);
    t.join();
}

TEST(FrameQueueTest, ClearEmptiesQueue) {
    FrameQueue q(5);
    q.push(make_dummy_frame());
    q.push(make_dummy_frame());
    q.clear();
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0);
}

TEST(FrameQueueTest, MaxSizeEvictsOldest) {
    FrameQueue q(2);
    auto f1 = make_dummy_frame();
    f1.pts = 1.0;
    auto f2 = make_dummy_frame();
    f2.pts = 2.0;
    auto f3 = make_dummy_frame();
    f3.pts = 3.0;

    q.push(std::move(f1));
    q.push(std::move(f2));
    q.push(std::move(f3)); // should evict f1

    EXPECT_EQ(q.size(), 2);
    auto popped = q.pop();
    EXPECT_EQ(popped.pts, 2.0);
}

TEST(FrameQueueTest, DrainUnblocksPoppers) {
    FrameQueue q;
    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        q.drain();
    });

    auto f = q.pop();
    EXPECT_EQ(f.width, 0);
    t.join();
}

TEST(FrameQueueTest, TryPopReturnsFalseOnTimeout) {
    FrameQueue q;
    VideoFrame f;
    EXPECT_FALSE(q.try_pop(f, 10));
}

#include <gtest/gtest.h>
#include <audio/AudioQueue.h>
#include <thread>

using namespace rav;

static AudioFrame make_dummy_audio_frame() {
    AudioFrame f;
    f.sample_rate = 44100;
    f.channels = 2;
    f.nb_samples = 1024;
    f.pts = 0.0;
    return f;
}

TEST(AudioQueueTest, InitiallyEmpty) {
    AudioQueue q;
    EXPECT_TRUE(q.empty());
}

TEST(AudioQueueTest, PushAndSize) {
    AudioQueue q(10);
    EXPECT_TRUE(q.push(make_dummy_audio_frame()));
    EXPECT_EQ(q.size(), 1);
}

TEST(AudioQueueTest, FullQueueRejects) {
    AudioQueue q(2);
    EXPECT_TRUE(q.push(make_dummy_audio_frame()));
    EXPECT_TRUE(q.push(make_dummy_audio_frame()));
    EXPECT_FALSE(q.push(make_dummy_audio_frame()));
}

TEST(AudioQueueTest, PopReturnsFrame) {
    AudioQueue q;
    auto f = make_dummy_audio_frame();
    f.pts = 7.0;
    q.push(std::move(f));

    auto popped = q.pop();
    EXPECT_EQ(popped.pts, 7.0);
    EXPECT_EQ(popped.sample_rate, 44100);
}

TEST(AudioQueueTest, Clear) {
    AudioQueue q;
    q.push(make_dummy_audio_frame());
    q.push(make_dummy_audio_frame());
    q.clear();
    EXPECT_TRUE(q.empty());
}

TEST(AudioQueueTest, DrainUnblocks) {
    AudioQueue q;
    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        q.drain();
    });

    auto f = q.pop();
    EXPECT_EQ(f.sample_rate, 0);
    t.join();
}

TEST(AudioQueueTest, TryPopTimeout) {
    AudioQueue q;
    AudioFrame f;
    EXPECT_FALSE(q.try_pop(f, 10));
}

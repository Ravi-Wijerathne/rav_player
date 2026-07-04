#include <gtest/gtest.h>
#include <audio/AudioClock.h>
#include <thread>

using namespace rav;

TEST(AudioClockTest, InitiallyStopped) {
    AudioClock ac;
    EXPECT_FALSE(ac.is_running());
}

TEST(AudioClockTest, StartAndElapsed) {
    AudioClock ac;
    ac.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_GE(ac.elapsed(), 0.015);
    EXPECT_TRUE(ac.is_running());
}

TEST(AudioClockTest, PauseAndResume) {
    AudioClock ac;
    ac.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ac.pause();
    double paused = ac.elapsed();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_DOUBLE_EQ(ac.elapsed(), paused);
    EXPECT_FALSE(ac.is_running());

    ac.resume();
    EXPECT_TRUE(ac.is_running());
}

TEST(AudioClockTest, SetPts) {
    AudioClock ac;
    ac.set_pts(5.0);
    EXPECT_GE(ac.pts(), 5.0);
    EXPECT_TRUE(ac.is_running());
}

TEST(AudioClockTest, Reset) {
    AudioClock ac;
    ac.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ac.reset();
    EXPECT_DOUBLE_EQ(ac.pts(), 0.0);
    EXPECT_FALSE(ac.is_running());
}

TEST(AudioClockTest, BytesPerSecond) {
    AudioClock ac;
    ac.set_bytes_per_second(44100.0 * 2 * 2);
    EXPECT_DOUBLE_EQ(ac.bytes_per_second(), 44100.0 * 2 * 2);
}

TEST(AudioClockTest, Serial) {
    AudioClock ac;
    ac.set_serial(3.0);
    EXPECT_DOUBLE_EQ(ac.serial(), 3.0);
}

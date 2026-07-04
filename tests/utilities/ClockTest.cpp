#include <gtest/gtest.h>
#include <utilities/Clock.h>
#include <thread>

using namespace rav;

TEST(ClockTest, InitiallyStopped) {
    Clock clock;
    EXPECT_FALSE(clock.is_running());
    EXPECT_DOUBLE_EQ(clock.elapsed(), 0.0);
}

TEST(ClockTest, ElapsedIncreases) {
    Clock clock;
    clock.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    double t = clock.elapsed();
    EXPECT_GE(t, 0.04);
    EXPECT_LE(t, 0.5);
}

TEST(ClockTest, PauseAndResume) {
    Clock clock;
    clock.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    clock.pause();
    double paused_at = clock.elapsed();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_DOUBLE_EQ(clock.elapsed(), paused_at);

    clock.resume();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_GT(clock.elapsed(), paused_at);
}

TEST(ClockTest, Reset) {
    Clock clock;
    clock.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    clock.reset();
    EXPECT_FALSE(clock.is_running());
    EXPECT_DOUBLE_EQ(clock.elapsed(), 0.0);
}

TEST(ClockTest, Set) {
    Clock clock;
    clock.start();
    clock.set(10.0);
    EXPECT_GE(clock.elapsed(), 9.9);
}

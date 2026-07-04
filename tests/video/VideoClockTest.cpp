#include <gtest/gtest.h>
#include <video/VideoClock.h>
#include <thread>

using namespace rav;

TEST(VideoClockTest, InitiallyZero) {
    VideoClock vc;
    EXPECT_DOUBLE_EQ(vc.pts(), 0.0);
    EXPECT_DOUBLE_EQ(vc.serial(), 0.0);
}

TEST(VideoClockTest, SetPts) {
    VideoClock vc;
    vc.set_pts(10.0, 1.0);
    EXPECT_DOUBLE_EQ(vc.pts(), 10.0);
    EXPECT_DOUBLE_EQ(vc.serial(), 1.0);
}

TEST(VideoClockTest, Reset) {
    VideoClock vc;
    vc.set_pts(10.0, 1.0);
    vc.reset();
    EXPECT_DOUBLE_EQ(vc.pts(), 0.0);
    EXPECT_DOUBLE_EQ(vc.serial(), 0.0);
}

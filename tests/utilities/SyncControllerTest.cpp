#include <gtest/gtest.h>
#include "utilities/SyncController.h"
#include <thread>
#include <chrono>

using namespace rav;

TEST(SyncControllerTest, DefaultState) {
    SyncController ctrl;
    EXPECT_DOUBLE_EQ(ctrl.elapsed_seconds(), 0.0);
    EXPECT_DOUBLE_EQ(ctrl.compute_drift(0.0), 0.0);
    EXPECT_DOUBLE_EQ(ctrl.correction_factor(0.0), 1.0);
    EXPECT_DOUBLE_EQ(ctrl.drift_integral(), 0.0);
}

TEST(SyncControllerTest, AfterStartElapsedIncreases) {
    SyncController ctrl;
    ctrl.start();
    EXPECT_GE(ctrl.elapsed_seconds(), 0.0);
}

TEST(SyncControllerTest, ComputeDriftAfterStart) {
    SyncController ctrl;
    ctrl.start();
    double drift = ctrl.compute_drift(5.0);
    EXPECT_GT(drift, 0.0);
}

TEST(SyncControllerTest, CorrectionFactorWhenNotRunning) {
    SyncController ctrl;
    double factor = ctrl.correction_factor(0.5);
    EXPECT_DOUBLE_EQ(factor, 1.0);
}

TEST(SyncControllerTest, PauseAndResume) {
    SyncController ctrl;
    ctrl.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ctrl.pause();
    double elapsed_before = ctrl.elapsed_seconds();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    double elapsed_after = ctrl.elapsed_seconds();
    EXPECT_DOUBLE_EQ(elapsed_before, elapsed_after);
    ctrl.resume();
    EXPECT_GE(ctrl.elapsed_seconds(), elapsed_after);
}

TEST(SyncControllerTest, ResetClearsState) {
    SyncController ctrl;
    ctrl.start();
    ctrl.reset();
    EXPECT_DOUBLE_EQ(ctrl.elapsed_seconds(), 0.0);
    EXPECT_DOUBLE_EQ(ctrl.compute_drift(1.0), 0.0);
    EXPECT_DOUBLE_EQ(ctrl.drift_integral(), 0.0);
}

TEST(SyncControllerTest, CorrectionFactorReturnsOneInitially) {
    SyncController ctrl;
    ctrl.start();
    double factor = ctrl.correction_factor(0.5);
    EXPECT_DOUBLE_EQ(factor, 1.0);
}

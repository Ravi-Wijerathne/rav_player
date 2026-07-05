#include <gtest/gtest.h>
#include <core/PlaybackEngine.h>

using namespace rav;

TEST(PlaybackEngineTest, InitialStateIsIdle) {
    PlaybackEngine engine;
    EXPECT_EQ(engine.state(), PlayerState::Idle);
}

TEST(PlaybackEngineTest, InitialValues) {
    PlaybackEngine engine;
    EXPECT_DOUBLE_EQ(engine.duration(), 0.0);
    EXPECT_DOUBLE_EQ(engine.current_time(), 0.0);
    EXPECT_FALSE(engine.has_video());
    EXPECT_FALSE(engine.has_audio());
    EXPECT_FLOAT_EQ(engine.volume(), 1.0f);
}

TEST(PlaybackEngineTest, EventBusExists) {
    PlaybackEngine engine;
    EXPECT_NO_FATAL_FAILURE(engine.event_bus());
}

TEST(PlaybackEngineTest, SetVolume) {
    PlaybackEngine engine;
    engine.set_volume(0.5f);
    EXPECT_FLOAT_EQ(engine.volume(), 0.5f);
    engine.set_volume(0.0f);
    EXPECT_FLOAT_EQ(engine.volume(), 0.0f);
    engine.set_volume(2.0f);
    EXPECT_FLOAT_EQ(engine.volume(), 2.0f);
}

TEST(PlaybackEngineTest, SeekBeforeOpen) {
    PlaybackEngine engine;
    EXPECT_NO_FATAL_FAILURE(engine.seek(10.0));
}

TEST(PlaybackEngineTest, PauseBeforePlay) {
    PlaybackEngine engine;
    EXPECT_NO_FATAL_FAILURE(engine.pause());
}

TEST(PlaybackEngineTest, StopBeforePlay) {
    PlaybackEngine engine;
    EXPECT_NO_FATAL_FAILURE(engine.stop());
}

TEST(PlaybackEngineTest, CloseBeforeOpen) {
    PlaybackEngine engine;
    EXPECT_NO_FATAL_FAILURE(engine.close());
}

TEST(PlaybackEngineTest, OpenNonexistentFileReturnsFalse) {
    PlaybackEngine engine;
    bool result = engine.open("/nonexistent/file.mp4");
    EXPECT_FALSE(result);
}

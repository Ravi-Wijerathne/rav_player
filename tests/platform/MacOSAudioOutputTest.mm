#include <gtest/gtest.h>

#import <AudioToolbox/AudioToolbox.h>

#include "platform/MacOSAudioOutput.h"

using namespace rav;

TEST(MacOSAudioOutputTest, CreateAndDestroy) {
    MacOSAudioOutput output;
    SUCCEED();
}

TEST(MacOSAudioOutputTest, NotInitializedByDefault) {
    MacOSAudioOutput output;
    EXPECT_FALSE(output.is_initialized());
}

TEST(MacOSAudioOutputTest, NotPlayingByDefault) {
    MacOSAudioOutput output;
    EXPECT_FALSE(output.is_playing());
}

TEST(MacOSAudioOutputTest, Latency) {
    MacOSAudioOutput output;
    EXPECT_DOUBLE_EQ(output.latency(), 0.05);
}

TEST(MacOSAudioOutputTest, StopBeforeInit) {
    MacOSAudioOutput output;
    EXPECT_FALSE(output.stop());
}

TEST(MacOSAudioOutputTest, InitWithDefaultSpec) {
    MacOSAudioOutput output;
    AudioOutputSpec spec;
    bool result = output.init(spec);
    EXPECT_TRUE(result);
    EXPECT_TRUE(output.is_initialized());
}

TEST(MacOSAudioOutputTest, InitWithCustomSpec) {
    MacOSAudioOutput output;
    AudioOutputSpec spec;
    spec.sample_rate = 44100;
    spec.channels = 2;
    spec.format = AV_SAMPLE_FMT_S16;
    bool result = output.init(spec);
    EXPECT_TRUE(result);
    EXPECT_TRUE(output.is_initialized());
}

TEST(MacOSAudioOutputTest, PlayAfterInit) {
    MacOSAudioOutput output;
    AudioOutputSpec spec;
    ASSERT_TRUE(output.init(spec));
    EXPECT_TRUE(output.play());
    EXPECT_TRUE(output.is_playing());
}

TEST(MacOSAudioOutputTest, PauseAndResume) {
    MacOSAudioOutput output;
    AudioOutputSpec spec;
    ASSERT_TRUE(output.init(spec));
    ASSERT_TRUE(output.play());
    EXPECT_TRUE(output.is_playing());
    EXPECT_TRUE(output.pause());
    EXPECT_FALSE(output.is_playing());
    EXPECT_TRUE(output.resume());
    EXPECT_TRUE(output.is_playing());
}

TEST(MacOSAudioOutputTest, ShutdownStopsPlayback) {
    MacOSAudioOutput output;
    AudioOutputSpec spec;
    ASSERT_TRUE(output.init(spec));
    ASSERT_TRUE(output.play());
    EXPECT_TRUE(output.is_playing());
    output.shutdown();
    EXPECT_FALSE(output.is_initialized());
    EXPECT_FALSE(output.is_playing());
}

TEST(MacOSAudioOutputTest, WriteFramesWithFillCallback) {
    MacOSAudioOutput output;
    int callback_calls = 0;
    output.set_fill_callback([&callback_calls](uint8_t* data, int frames) -> int {
        ++callback_calls;
        return frames;
    });
    uint8_t buffer[1024] = {};
    int written = output.write_frames(buffer, 256);
    EXPECT_EQ(written, 256);
    EXPECT_EQ(callback_calls, 1);
}

TEST(MacOSAudioOutputTest, DestructorShutsDown) {
    {
        MacOSAudioOutput output;
        AudioOutputSpec spec;
        ASSERT_TRUE(output.init(spec));
        EXPECT_TRUE(output.is_initialized());
    }
    SUCCEED();
}

#include <gtest/gtest.h>

extern "C" {
#include <libavutil/samplefmt.h>
}

#include <audio/AudioMixer.h>

using namespace rav;

TEST(AudioMixerTest, ApplyGain) {
    float buf[4] = {1.0f, -1.0f, 0.5f, -0.5f};
    AudioMixer::apply_gain(buf, 4, 0.5f);
    EXPECT_FLOAT_EQ(buf[0], 0.5f);
    EXPECT_FLOAT_EQ(buf[1], -0.5f);
    EXPECT_FLOAT_EQ(buf[2], 0.25f);
    EXPECT_FLOAT_EQ(buf[3], -0.25f);
}

TEST(AudioMixerTest, Silence) {
    float buf[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    AudioMixer::silence(buf, 4);
    for (int i = 0; i < 4; ++i) {
        EXPECT_FLOAT_EQ(buf[i], 0.0f);
    }
}

TEST(AudioMixerTest, SampleS16ToFloat) {
    int16_t samples[] = {0, 16384, 32767, -32768};
    EXPECT_NEAR(AudioMixer::sample_to_float(
        reinterpret_cast<const uint8_t*>(samples), AV_SAMPLE_FMT_S16, 0), 0.0f, 0.001f);
    EXPECT_NEAR(AudioMixer::sample_to_float(
        reinterpret_cast<const uint8_t*>(samples), AV_SAMPLE_FMT_S16, 1), 0.5f, 0.001f);
    EXPECT_NEAR(AudioMixer::sample_to_float(
        reinterpret_cast<const uint8_t*>(samples), AV_SAMPLE_FMT_S16, 2), 1.0f, 0.001f);
    EXPECT_NEAR(AudioMixer::sample_to_float(
        reinterpret_cast<const uint8_t*>(samples), AV_SAMPLE_FMT_S16, 3), -1.0f, 0.001f);
}

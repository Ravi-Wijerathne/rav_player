#include <gtest/gtest.h>
#include <audio/AudioFrame.h>

using namespace rav;

TEST(AudioFrameTest, DefaultFrame) {
    AudioFrame f;
    EXPECT_EQ(f.sample_rate, 0);
    EXPECT_EQ(f.channels, 0);
    EXPECT_EQ(f.nb_samples, 0);
    EXPECT_DOUBLE_EQ(f.pts, 0.0);
}

TEST(AudioFrameTest, DurationFromFrameNull) {
    EXPECT_DOUBLE_EQ(AudioFrame::duration_from_frame(nullptr), 0.0);
}

TEST(AudioFrameTest, DurationFromSampleRate) {
    // Create a real AVFrame to test the calculation
    auto avf = av_frame_alloc();
    ASSERT_NE(avf, nullptr);
    avf->sample_rate = 44100;
    avf->nb_samples = 1024;

    EXPECT_NEAR(AudioFrame::duration_from_frame(avf), 1024.0 / 44100.0, 0.0001);
    av_frame_free(&avf);
}

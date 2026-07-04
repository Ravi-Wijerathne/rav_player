#include <gtest/gtest.h>
#include <video/VideoFrame.h>

using namespace rav;

TEST(VideoFrameTest, DefaultFrame) {
    VideoFrame f;
    EXPECT_EQ(f.width, 0);
    EXPECT_EQ(f.height, 0);
    EXPECT_DOUBLE_EQ(f.pts, 0.0);
    EXPECT_EQ(f.format, VideoFrameFormat::Unknown);
    EXPECT_FALSE(f.is_hardware);
}

TEST(VideoFrameTest, FormatConversionYUV420P) {
    EXPECT_EQ(VideoFrame::from_av_pixel_format(AV_PIX_FMT_YUV420P),
              VideoFrameFormat::YUV420P);
}

TEST(VideoFrameTest, FormatConversionNV12) {
    EXPECT_EQ(VideoFrame::from_av_pixel_format(AV_PIX_FMT_NV12),
              VideoFrameFormat::NV12);
}

TEST(VideoFrameTest, FormatConversionRGBA) {
    EXPECT_EQ(VideoFrame::from_av_pixel_format(AV_PIX_FMT_RGBA),
              VideoFrameFormat::RGBA);
}

TEST(VideoFrameTest, FormatConversionUnknown) {
    EXPECT_EQ(VideoFrame::from_av_pixel_format(AV_PIX_FMT_NONE),
              VideoFrameFormat::Unknown);
}

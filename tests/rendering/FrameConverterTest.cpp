#include <gtest/gtest.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
}

#include <rendering/FrameConverter.h>

using namespace rav;

TEST(FrameConverterTest, InitiallyNotOpen) {
    FrameConverter fc;
    EXPECT_FALSE(fc.is_open());
}

TEST(FrameConverterTest, InitAndClose) {
    FrameConverter fc;
    FrameConverterSpec spec;
    spec.src_width = 320;
    spec.src_height = 240;
    spec.src_format = AV_PIX_FMT_YUV420P;
    spec.dst_width = 320;
    spec.dst_height = 240;
    spec.dst_format = AV_PIX_FMT_RGBA;

    ASSERT_TRUE(fc.init(spec));
    EXPECT_TRUE(fc.is_open());
    EXPECT_EQ(fc.spec().src_width, 320);
    EXPECT_EQ(fc.spec().dst_format, AV_PIX_FMT_RGBA);
    EXPECT_GT(fc.data_size(), 0);

    fc.close();
    EXPECT_FALSE(fc.is_open());
}

TEST(FrameConverterTest, ConvertDummyFrame) {
    FrameConverter fc;
    FrameConverterSpec spec;
    spec.src_width = 64;
    spec.src_height = 48;
    spec.src_format = AV_PIX_FMT_YUV420P;
    spec.dst_width = 64;
    spec.dst_height = 48;
    spec.dst_format = AV_PIX_FMT_RGBA;

    ASSERT_TRUE(fc.init(spec));

    auto* frame = av_frame_alloc();
    ASSERT_NE(frame, nullptr);
    frame->width = 64;
    frame->height = 48;
    frame->format = AV_PIX_FMT_YUV420P;

    int ret = av_image_alloc(frame->data, frame->linesize,
                             frame->width, frame->height,
                             static_cast<AVPixelFormat>(frame->format), 32);
    ASSERT_GE(ret, 0);

    EXPECT_TRUE(fc.convert_to_buffer(frame));
    EXPECT_GT(fc.data_size(), 0);

    av_freep(&frame->data[0]);
    av_frame_free(&frame);
}

TEST(FrameConverterTest, MoveConstructor) {
    FrameConverter fc;
    FrameConverterSpec spec;
    spec.src_width = 100;
    spec.src_height = 50;
    spec.src_format = AV_PIX_FMT_NV12;
    spec.dst_width = 100;
    spec.dst_height = 50;
    spec.dst_format = AV_PIX_FMT_BGRA;

    ASSERT_TRUE(fc.init(spec));

    FrameConverter fc2 = std::move(fc);
    EXPECT_TRUE(fc2.is_open());
    EXPECT_FALSE(fc.is_open());
    EXPECT_EQ(fc2.spec().src_width, 100);
}

TEST(FrameConverterTest, InvalidInit) {
    FrameConverter fc;
    FrameConverterSpec spec;
    spec.src_width = 0;
    spec.src_height = 0;
    spec.src_format = AV_PIX_FMT_NONE;

    EXPECT_FALSE(fc.init(spec));
    EXPECT_FALSE(fc.is_open());
}

TEST(FrameConverterTest, ConvertWithNullInput) {
    FrameConverter fc;
    FrameConverterSpec spec;
    spec.src_width = 64;
    spec.src_height = 48;
    spec.src_format = AV_PIX_FMT_YUV420P;
    spec.dst_width = 64;
    spec.dst_height = 48;
    spec.dst_format = AV_PIX_FMT_RGBA;
    ASSERT_TRUE(fc.init(spec));

    EXPECT_FALSE(fc.convert_to_buffer(nullptr));
}

TEST(FrameConverterTest, ResizeConversion) {
    FrameConverter fc;
    FrameConverterSpec spec;
    spec.src_width = 128;
    spec.src_height = 72;
    spec.src_format = AV_PIX_FMT_YUV420P;
    spec.dst_width = 64;
    spec.dst_height = 36;
    spec.dst_format = AV_PIX_FMT_RGBA;

    ASSERT_TRUE(fc.init(spec));
    EXPECT_EQ(spec.dst_width, 64);
    EXPECT_EQ(spec.dst_height, 36);
    EXPECT_EQ(fc.data_size(), 64 * 36 * 4);
}

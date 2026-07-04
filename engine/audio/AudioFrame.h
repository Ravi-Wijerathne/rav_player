#pragma once

#include <memory>
#include <vector>

#include "../ffmpeg/FFmpegContext.h"

namespace rav {

struct AudioFrame {
    FramePtr frame;
    double pts{0.0};
    double duration{0.0};
    int sample_rate{0};
    int channels{0};
    AVSampleFormat format{AV_SAMPLE_FMT_NONE};
    int64_t channel_layout{0};
    int nb_samples{0};

    std::vector<uint8_t> planar_data;

    static double duration_from_frame(const AVFrame* f) {
        if (!f || !f->sample_rate) return 0.0;
        return static_cast<double>(f->nb_samples) / f->sample_rate;
    }
};

} // namespace rav

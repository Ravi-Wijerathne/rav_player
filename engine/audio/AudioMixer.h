#pragma once

#include <cstdint>
#include <vector>

extern "C" {
#include <libavutil/samplefmt.h>
}

namespace rav {

class AudioMixer {
public:
    static float sample_to_float(const uint8_t* data, AVSampleFormat fmt,
                                 int index) {
        switch (fmt) {
            case AV_SAMPLE_FMT_U8:
            case AV_SAMPLE_FMT_U8P:
                return (data[index] / 128.0f) - 1.0f;
            case AV_SAMPLE_FMT_S16:
            case AV_SAMPLE_FMT_S16P:
                return reinterpret_cast<const int16_t*>(data)[index] / 32768.0f;
            case AV_SAMPLE_FMT_S32:
            case AV_SAMPLE_FMT_S32P:
                return reinterpret_cast<const int32_t*>(data)[index] / 2147483648.0f;
            case AV_SAMPLE_FMT_FLT:
            case AV_SAMPLE_FMT_FLTP:
                return reinterpret_cast<const float*>(data)[index];
            case AV_SAMPLE_FMT_DBL:
            case AV_SAMPLE_FMT_DBLP:
                return static_cast<float>(reinterpret_cast<const double*>(data)[index]);
            default:
                return 0.0f;
        }
    }

    template <typename T>
    static void mix_add(T* buffer, const T* source, int samples,
                        float gain = 1.0f) {
        for (int i = 0; i < samples; ++i) {
            buffer[i] += static_cast<T>(source[i] * gain);
        }
    }

    static void apply_gain(float* buffer, int samples, float gain) {
        for (int i = 0; i < samples; ++i) {
            buffer[i] *= gain;
        }
    }

    static void silence(float* buffer, int samples) {
        std::fill(buffer, buffer + samples, 0.0f);
    }
};

} // namespace rav

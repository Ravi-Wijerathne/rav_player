#pragma once

#include <string>
#include <vector>

namespace rav {

enum class SubtitleFormat {
    Unknown,
    SRT,
    ASS,
    SSA,
    WebVTT
};

struct SubtitleFrame {
    double start_time{0.0};
    double end_time{0.0};
    std::string text;
    std::string style;

    int x{0};
    int y{0};
    int width{0};
    int height{0};

    bool is_bitmap{false};
    std::vector<uint8_t> bitmap_data;

    bool is_valid() const {
        return !text.empty() || is_bitmap;
    }
};

} // namespace rav

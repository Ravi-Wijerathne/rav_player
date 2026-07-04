#pragma once

#include <memory>
#include <string>
#include <vector>

#include "SubtitleFrame.h"

namespace rav {

class SubtitleParser {
public:
    virtual ~SubtitleParser() = default;

    virtual bool load(const std::string& content) = 0;
    virtual bool load_file(const std::string& path) = 0;
    virtual std::vector<SubtitleFrame> parse() = 0;
    virtual SubtitleFormat format() const = 0;

    std::vector<SubtitleFrame> frames_at_time(double seconds) const {
        std::vector<SubtitleFrame> result;
        for (const auto& frame : frames_) {
            if (seconds >= frame.start_time && seconds < frame.end_time) {
                result.push_back(frame);
            }
        }
        return result;
    }

protected:
    std::vector<SubtitleFrame> frames_;
};

} // namespace rav

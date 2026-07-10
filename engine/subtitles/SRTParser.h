#pragma once

#include <string>
#include <vector>

#include "SubtitleParser.h"

namespace rav {

class SRTParser : public SubtitleParser {
public:
    bool load(const std::string& content) override;
    bool load_file(const std::string& path) override;
    std::vector<SubtitleFrame> parse() override;
    SubtitleFormat format() const override;

private:
    static double time_to_seconds(int h, int m, int s, int ms);

    std::string raw_content_;
};

} // namespace rav

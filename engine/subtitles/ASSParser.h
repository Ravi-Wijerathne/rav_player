#pragma once

#include <string>
#include <vector>

#include "SubtitleParser.h"

namespace rav {

class ASSParser : public SubtitleParser {
public:
    bool load(const std::string& content) override;
    bool load_file(const std::string& path) override;
    std::vector<SubtitleFrame> parse() override;
    SubtitleFormat format() const override;

private:
    static std::string strip_ass_overrides(const std::string& text);

    std::string raw_content_;
};

} // namespace rav

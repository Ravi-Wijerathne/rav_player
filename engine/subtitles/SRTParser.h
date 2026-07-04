#pragma once

#include <regex>
#include <sstream>
#include <string>

#include "SubtitleParser.h"

namespace rav {

class SRTParser : public SubtitleParser {
public:
    bool load(const std::string& content) override {
        raw_content_ = content;
        return true;
    }

    bool load_file(const std::string& path) override {
        FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) return false;
        std::fseek(f, 0, SEEK_END);
        long size = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        std::string buf;
        buf.resize(size);
        std::fread(buf.data(), 1, size, f);
        std::fclose(f);
        raw_content_ = std::move(buf);
        return true;
    }

    std::vector<SubtitleFrame> parse() override {
        frames_.clear();
        if (raw_content_.empty()) return frames_;

        std::istringstream stream(raw_content_);
        std::string line;

        static const std::regex time_regex(
            R"((\d{2}):(\d{2}):(\d{2})[,\.](\d{3})\s*-->\s*(\d{2}):(\d{2}):(\d{2})[,\.](\d{3}))");

        while (std::getline(stream, line)) {
            if (line.empty()) continue;

            std::smatch match;
            if (!std::regex_search(line, match, time_regex)) continue;

            SubtitleFrame frame;
            frame.start_time = time_to_seconds(
                std::stoi(match[1]), std::stoi(match[2]),
                std::stoi(match[3]), std::stoi(match[4]));
            frame.end_time = time_to_seconds(
                std::stoi(match[5]), std::stoi(match[6]),
                std::stoi(match[7]), std::stoi(match[8]));

            std::string text_line;
            while (std::getline(stream, text_line) && !text_line.empty()) {
                if (!frame.text.empty()) frame.text += "\n";
                frame.text += text_line;
            }

            frames_.push_back(std::move(frame));
        }

        return frames_;
    }

    SubtitleFormat format() const override { return SubtitleFormat::SRT; }

private:
    static double time_to_seconds(int h, int m, int s, int ms) {
        return h * 3600.0 + m * 60.0 + s + ms / 1000.0;
    }

    std::string raw_content_;
};

} // namespace rav

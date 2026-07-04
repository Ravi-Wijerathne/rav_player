#pragma once

#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "SubtitleParser.h"

namespace rav {

class ASSParser : public SubtitleParser {
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

        static const std::regex dialogue_regex(
            R"(Dialogue:\s*(\d+),(\d+):(\d+):(\d+)\.(\d+),(\d+):(\d+):(\d+)\.(\d+),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),([^,]*),(.*))");

        std::istringstream stream(raw_content_);
        std::string line;

        while (std::getline(stream, line)) {
            std::smatch match;
            if (!std::regex_match(line, match, dialogue_regex)) continue;

            SubtitleFrame frame;
            int h1 = std::stoi(match[2]);
            int m1 = std::stoi(match[3]);
            int s1 = std::stoi(match[4]);
            int cs1 = std::stoi(match[5]);
            frame.start_time = h1 * 3600.0 + m1 * 60.0 + s1 + cs1 / 100.0;

            int h2 = std::stoi(match[6]);
            int m2 = std::stoi(match[7]);
            int s2 = std::stoi(match[8]);
            int cs2 = std::stoi(match[9]);
            frame.end_time = h2 * 3600.0 + m2 * 60.0 + s2 + cs2 / 100.0;

            frame.style = match[10];
            std::string raw_text = match[16];

            frame.text = strip_ass_overrides(raw_text);
            frames_.push_back(std::move(frame));
        }

        return frames_;
    }

    SubtitleFormat format() const override {
        return SubtitleFormat::ASS;
    }

private:
    static std::string strip_ass_overrides(const std::string& text) {
        std::string result;
        result.reserve(text.size());
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '{') {
                while (i < text.size() && text[i] != '}') ++i;
                continue;
            }
            if (text[i] == '\\' && i + 1 < text.size()) {
                if (text[i + 1] == 'N' || text[i + 1] == 'n') {
                    result += '\n';
                    ++i;
                    continue;
                }
                if (text[i + 1] == 'h') {
                    result += ' ';
                    ++i;
                    continue;
                }
            }
            result += text[i];
        }
        return result;
    }

    std::string raw_content_;
};

} // namespace rav

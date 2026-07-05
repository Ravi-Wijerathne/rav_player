#pragma once

#include <regex>
#include <sstream>
#include <string>

#include "SubtitleParser.h"

namespace rav {

class VttParser : public SubtitleParser {
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

        // Skip the WEBVTT header if present
        if (std::getline(stream, line)) {
            // Line starts with "WEBVTT" (possibly with BOM)
            // Just skip it and any blank lines after
        }

        // Skip blank lines and optional header metadata
        while (std::getline(stream, line)) {
            if (!line.empty() && line.find("-->") == std::string::npos) {
                continue; // Skip cue identifier, comments, etc.
            }
            if (line.find("-->") != std::string::npos) break;
        }

        // Process first cue if we found a timestamp line
        if (line.find("-->") != std::string::npos) {
            parse_cue(line, stream);
        }

        // Process remaining cues
        while (std::getline(stream, line)) {
            if (line.empty() || line.find("NOTE") == 0 || line.find("STYLE") == 0) {
                continue;
            }
            if (line.find("-->") != std::string::npos) {
                parse_cue(line, stream);
            }
        }

        return frames_;
    }

    SubtitleFormat format() const override { return SubtitleFormat::WebVTT; }

private:
    static double vtt_timestamp_to_seconds(const std::string& ts) {
        // Handle both HH:MM:SS.mmm and MM:SS.mmm
        int h = 0, m = 0, s = 0, ms = 0;
        int parsed = std::sscanf(ts.c_str(), "%d:%d:%d.%d", &h, &m, &s, &ms);
        if (parsed == 4) {
            return h * 3600.0 + m * 60.0 + s + ms / 1000.0;
        }
        // Try MM:SS.mmm
        parsed = std::sscanf(ts.c_str(), "%d:%d.%d", &m, &s, &ms);
        if (parsed == 3) {
            return m * 60.0 + s + ms / 1000.0;
        }
        return 0.0;
    }

    void parse_cue(const std::string& timing_line, std::istringstream& stream) {
        static const std::regex time_regex(
            R"((\d{1,2}:?\d{2}:\d{2}\.\d{3}|\d{1,2}:\d{2}\.\d{3})\s*-->\s*(\d{1,2}:?\d{2}:\d{2}\.\d{3}|\d{1,2}:\d{2}\.\d{3}))");

        std::smatch match;
        if (!std::regex_search(timing_line, match, time_regex)) return;

        SubtitleFrame frame;
        frame.start_time = vtt_timestamp_to_seconds(match[1]);
        frame.end_time = vtt_timestamp_to_seconds(match[2]);

        // Read cue payload until blank line or EOF
        std::string text_line;
        while (std::getline(stream, text_line)) {
            if (text_line.empty()) break;
            // Skip cue settings lines (contain ':' after a cue)
            if (!frame.text.empty() || !text_line.empty()) {
                if (!frame.text.empty()) frame.text += "\n";
                frame.text += text_line;
            }
        }

        // Remove trailing newline
        if (!frame.text.empty() && frame.text.back() == '\n') {
            frame.text.pop_back();
        }

        frames_.push_back(std::move(frame));
    }

    std::string raw_content_;
};

} // namespace rav

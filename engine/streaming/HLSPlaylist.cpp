#include "HLSPlaylist.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <system_error>

namespace rav {

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        parts.push_back(trim(item));
    }
    return parts;
}

static std::string get_value(const std::string& line, const std::string& key) {
    auto pos = line.find(key + "=");
    if (pos == std::string::npos) return {};
    auto start = pos + key.size() + 1;
    auto val = line.substr(start);
    if (val.front() == '"' && val.back() == '"') {
        val = val.substr(1, val.size() - 2);
    }
    return val;
}

static int parse_int(const std::string& s, int fallback = 0) {
    int result = fallback;
    std::from_chars(s.data(), s.data() + s.size(), result);
    return result;
}

static double parse_double(const std::string& s, double fallback = 0.0) {
    try {
        return std::stod(s);
    } catch (...) {
        return fallback;
    }
}

bool HLSPlaylistParser::is_hls_playlist(const std::string& content) const {
    return content.find("#EXTM3U") == 0;
}

bool HLSPlaylistParser::is_hls_url(const std::string& url) {
    auto pos = url.rfind('.');
    if (pos == std::string::npos) return false;
    auto ext = url.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext == ".m3u8" || ext == ".m3u";
}

HLSPlaylist HLSPlaylistParser::parse(const std::string& content) {
    HLSPlaylist playlist;

    if (!is_hls_playlist(content)) {
        return playlist;
    }

    std::stringstream ss(content);
    std::string line;
    bool has_stream_inf = false;
    int current_bandwidth = 0;
    int current_width = 0;
    int current_height = 0;

    while (std::getline(ss, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (line == "#EXTM3U") continue;

        if (line.rfind("#EXT-X-VERSION:", 0) == 0) {
            playlist.version = parse_int(line.substr(15));
            continue;
        }

        if (line.rfind("#EXT-X-TARGETDURATION:", 0) == 0) {
            playlist.target_duration = parse_double(line.substr(22));
            continue;
        }

        if (line.rfind("#EXT-X-MEDIA-SEQUENCE:", 0) == 0) {
            playlist.media_sequence = parse_int(line.substr(22));
            continue;
        }

        if (line == "#EXT-X-ENDLIST") {
            playlist.is_endless = false;
            continue;
        }

        if (line.rfind("#EXTINF:", 0) == 0) {
            auto duration_str = line.substr(8);
            auto comma = duration_str.find(',');
            auto title = std::string{};
            if (comma != std::string::npos) {
                title = trim(duration_str.substr(comma + 1));
                duration_str = duration_str.substr(0, comma);
            }
            HLSSegment seg;
            seg.duration = parse_double(duration_str);
            if (std::getline(ss, line)) {
                seg.uri = trim(line);
            }
            seg.sequence = playlist.media_sequence + playlist.segments.size();
            playlist.segments.push_back(seg);
            continue;
        }

        if (line.rfind("#EXT-X-STREAM-INF:", 0) == 0) {
            has_stream_inf = true;
            auto params = line.substr(18);
            auto parts = split(params, ',');
            for (const auto& p : parts) {
                if (p.rfind("BANDWIDTH", 0) == 0) {
                    current_bandwidth = parse_int(get_value(p, "BANDWIDTH"));
                }
                if (p.rfind("RESOLUTION", 0) == 0) {
                    auto res = get_value(p, "RESOLUTION");
                    auto x = res.find('x');
                    if (x != std::string::npos) {
                        current_width = parse_int(res.substr(0, x));
                        current_height = parse_int(res.substr(x + 1));
                    }
                }
            }
            continue;
        }

        if (has_stream_inf && line[0] != '#') {
            HLSPlaylist::VariantStream var;
            var.bandwidth = current_bandwidth;
            var.width = current_width;
            var.height = current_height;
            var.uri = line;
            playlist.variants.push_back(var);
            has_stream_inf = false;
            current_bandwidth = 0;
            current_width = 0;
            current_height = 0;
        }
    }

    playlist.is_master = !playlist.variants.empty();
    return playlist;
}

} // namespace rav

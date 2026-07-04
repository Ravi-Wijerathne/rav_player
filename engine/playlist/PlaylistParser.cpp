#include "PlaylistParser.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace rav {

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

PlaylistFormat PlaylistParser::detect_format(const std::string& content) {
    if (content.empty()) return PlaylistFormat::Unknown;

    auto first = content.find_first_not_of(" \t\r\n");

    if (first != std::string::npos) {
        if (content.substr(first, 7) == "#EXTM3U") {
            return PlaylistFormat::M3U8;
        }
        if (content.substr(first, 1) == "#") {
            return PlaylistFormat::M3U;
        }
        if (content.substr(first, 10) == "[playlist]") {
            return PlaylistFormat::PLS;
        }
    }

    return PlaylistFormat::Unknown;
}

Playlist PlaylistParser::parse(const std::string& content, PlaylistFormat format) {
    switch (format) {
        case PlaylistFormat::M3U:
        case PlaylistFormat::M3U8:
            return parse_m3u(content);
        case PlaylistFormat::PLS:
            return parse_pls(content);
        default:
            return {};
    }
}

Playlist PlaylistParser::parse_m3u(const std::string& content) {
    Playlist playlist("Untitled");

    std::stringstream ss(content);
    std::string line;
    std::string current_title;

    while (std::getline(ss, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (line.rfind("#EXTM3U", 0) == 0) continue;

        if (line.rfind("#EXTINF:", 0) == 0) {
            auto duration_str = line.substr(8);
            auto comma = duration_str.find(',');
            if (comma != std::string::npos) {
                current_title = trim(duration_str.substr(comma + 1));
            }
            continue;
        }

        if (line.rfind("#", 0) == 0) continue;

        PlaylistItem item;
        item.uri = line;
        item.title = current_title.empty() ? line : current_title;
        playlist.add_item(item);
        current_title.clear();
    }

    return playlist;
}

Playlist PlaylistParser::parse_pls(const std::string& content) {
    Playlist playlist("Untitled");
    int num_entries = 0;

    std::stringstream ss(content);
    std::string line;

    while (std::getline(ss, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '[') continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        auto key = trim(line.substr(0, eq));
        auto value = trim(line.substr(eq + 1));

        if (key.rfind("NumberOfEntries", 0) == 0) {
            try {
                num_entries = std::stoi(value);
            } catch (...) {}
            continue;
        }

        if (key.rfind("File", 0) == 0 || key.rfind("Title", 0) == 0) {
            auto num_start = key.find_first_of("0123456789");
            if (num_start == std::string::npos) continue;
            auto entry_num = std::stoi(key.substr(num_start));
            while (static_cast<int>(playlist.size()) < entry_num) {
                playlist.add_item({});
            }
            if (key.rfind("File", 0) == 0) {
                playlist.item(entry_num - 1).uri = value;
                if (playlist.item(entry_num - 1).title.empty()) {
                    playlist.item(entry_num - 1).title = value;
                }
            } else if (key.rfind("Title", 0) == 0) {
                playlist.item(entry_num - 1).title = value;
            }
        }
    }

    return playlist;
}

std::string PlaylistParser::serialize(const Playlist& playlist, PlaylistFormat format) {
    std::string result;

    switch (format) {
        case PlaylistFormat::M3U:
        case PlaylistFormat::M3U8:
            result = "#EXTM3U\n";
            for (const auto& item : playlist.items()) {
                result += "#EXTINF:-1," + item.title + "\n";
                result += item.uri + "\n";
            }
            break;
        case PlaylistFormat::PLS:
            result = "[playlist]\n";
            result += "NumberOfEntries=" + std::to_string(playlist.size()) + "\n";
            for (size_t i = 0; i < playlist.size(); ++i) {
                result += "File" + std::to_string(i + 1) + "=" + playlist.item(i).uri + "\n";
                result += "Title" + std::to_string(i + 1) + "=" + playlist.item(i).title + "\n";
            }
            result += "Version=2\n";
            break;
        default:
            break;
    }

    return result;
}

bool PlaylistParser::is_playlist_extension(const std::string& path) {
    auto pos = path.rfind('.');
    if (pos == std::string::npos) return false;
    auto ext = path.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext == ".m3u" || ext == ".m3u8" || ext == ".pls" || ext == ".xspf";
}

} // namespace rav

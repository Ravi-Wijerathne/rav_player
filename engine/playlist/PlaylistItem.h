#pragma once

#include <chrono>
#include <string>

namespace rav {

struct PlaylistItem {
    std::string uri;
    std::string title;
    std::string artist;
    std::string album;
    std::chrono::milliseconds duration{0};
    int64_t track_number{0};
    std::string media_type;
    std::string thumbnail_path;

    bool operator==(const PlaylistItem& other) const {
        return uri == other.uri;
    }

    bool operator!=(const PlaylistItem& other) const {
        return uri != other.uri;
    }
};

} // namespace rav

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "PlaylistItem.h"
#include "Playlist.h"

namespace rav {

enum class PlaylistFormat {
    Unknown,
    M3U,
    M3U8,
    PLS
};

class PlaylistParser {
public:
    PlaylistParser() = default;

    static PlaylistFormat detect_format(const std::string& content);

    Playlist parse(const std::string& content, PlaylistFormat format);

    Playlist parse_m3u(const std::string& content);
    Playlist parse_pls(const std::string& content);

    std::string serialize(const Playlist& playlist, PlaylistFormat format);

    static bool is_playlist_extension(const std::string& path);
};

} // namespace rav

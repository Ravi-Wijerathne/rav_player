#pragma once

#include <memory>
#include <string>
#include <vector>

namespace rav {

struct HLSSegment {
    double duration{0.0};
    std::string uri;
    int64_t sequence{0};
};

struct HLSPlaylist {
    bool is_master{false};
    int version{1};
    double target_duration{0.0};
    int64_t media_sequence{0};
    bool is_endless{true};

    struct VariantStream {
        int bandwidth{0};
        int width{0};
        int height{0};
        std::string uri;
    };

    std::vector<VariantStream> variants;
    std::vector<HLSSegment> segments;
};

class HLSPlaylistParser {
public:
    HLSPlaylistParser() = default;

    HLSPlaylist parse(const std::string& content);
    bool is_hls_playlist(const std::string& content) const;

    static bool is_hls_url(const std::string& url);
};

} // namespace rav

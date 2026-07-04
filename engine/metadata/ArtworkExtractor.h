#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace rav {

enum class ArtworkFormat {
    Unknown,
    JPEG,
    PNG,
    BMP,
    GIF
};

class ArtworkExtractor {
public:
    ArtworkExtractor() = default;
    ~ArtworkExtractor() = default;

    bool has_artwork(const std::string& file_path);

    std::vector<uint8_t> extract_artwork(const std::string& file_path);

    static ArtworkFormat detect_format(const std::vector<uint8_t>& data);

    static bool is_valid_image(const std::vector<uint8_t>& data);

private:
    std::vector<uint8_t> extract_apic(void* fmt_ctx);
};

} // namespace rav

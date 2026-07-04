#pragma once

#include <memory>
#include <string>

#include "MediaInfo.h"

namespace rav {

class MetadataExtractor {
public:
    MetadataExtractor() = default;
    ~MetadataExtractor() = default;

    MediaInfo extract(const std::string& file_path);
    MediaInfo extract(const std::string& uri, const std::vector<uint8_t>& header_data);

    static bool has_embedded_metadata(const std::string& file_path);

private:
    MediaInfo extract_from_format(void* fmt_ctx);
    void extract_stream_info(void* fmt_ctx, MediaInfo& info);
    void extract_metadata_tags(void* fmt_ctx, MediaInfo& info);
};

} // namespace rav

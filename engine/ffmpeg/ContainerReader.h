#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include "FFmpegContext.h"

namespace rav {

struct StreamInfo {
    int index{-1};
    AVMediaType type{AVMEDIA_TYPE_UNKNOWN};
    AVCodecID codec_id{AV_CODEC_ID_NONE};
    std::string codec_name;
    int64_t bit_rate{0};

    int width{0};
    int height{0};
    AVRational framerate{};

    int sample_rate{0};
    AVChannelLayout channel_layout{};

    AVRational time_base{};
    int64_t duration{0};
};

struct ContainerInfo {
    std::string filename;
    std::string format_name;
    int64_t duration{0};
    int64_t bit_rate{0};
    std::vector<StreamInfo> streams;
    bool has_video{false};
    bool has_audio{false};
    bool has_subtitles{false};
};

class ContainerReader {
public:
    ContainerReader();
    ~ContainerReader() = default;

    ContainerReader(const ContainerReader&) = delete;
    ContainerReader& operator=(const ContainerReader&) = delete;

    ContainerReader(ContainerReader&&) = default;
    ContainerReader& operator=(ContainerReader&&) = default;

    bool open(const std::string& url);
    void close();
    bool is_open() const;
    const ContainerInfo& info() const;
    AVFormatContext* context();
    PacketPtr read_packet();
    bool seek(int stream_index, int64_t timestamp, int flags = 0);
    bool seek_to_time(double seconds, int stream_index = -1);

private:
    void build_info();

    FormatContextPtr fmt_ctx_;
    std::unique_ptr<ContainerInfo> info_;
    PacketPtr current_packet_;
};

} // namespace rav

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace rav {

struct MediaStreamInfo {
    int index{-1};
    std::string codec;
    std::string codec_profile;
    std::string language;
    int width{0};
    int height{0};
    double frame_rate{0.0};
    int sample_rate{0};
    int channels{0};
    std::string channel_layout;
    int bit_rate{0};
};

struct MediaInfo {
    std::string file_path;
    std::string format_name;
    std::string format_long_name;
    std::chrono::milliseconds duration{0};
    int64_t bit_rate{0};
    int64_t file_size{0};

    std::string title;
    std::string artist;
    std::string album;
    std::string album_artist;
    std::string genre;
    std::string composer;
    std::string year;
    int track_number{0};
    int disc_number{0};
    std::string comment;
    std::string copyright;
    std::string encoder;
    std::string creation_time;

    std::vector<MediaStreamInfo> video_streams;
    std::vector<MediaStreamInfo> audio_streams;
    std::vector<MediaStreamInfo> subtitle_streams;

    bool has_video() const { return !video_streams.empty(); }
    bool has_audio() const { return !audio_streams.empty(); }
    bool has_subtitles() const { return !subtitle_streams.empty(); }
};

} // namespace rav

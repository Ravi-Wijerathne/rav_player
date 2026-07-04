#pragma once

#include <string_view>

#include "MediaSource.h"

namespace rav {

class StreamSource : public MediaSource {
public:
    explicit StreamSource(std::string url, MediaSourceType type)
        : url_(std::move(url)), type_(type) {}

    bool open() override {
        opened_ = true;
        return true;
    }

    void close() override { opened_ = false; }

    bool is_open() const override { return opened_; }

    const std::string& url() const override { return url_; }

    MediaSourceType type() const override { return type_; }

    static MediaSourceType detect_type(std::string_view url) {
        if (url.starts_with("http://"))  return MediaSourceType::HTTP;
        if (url.starts_with("https://")) return MediaSourceType::HTTPS;
        if (url.starts_with("rtsp://"))  return MediaSourceType::RTSP;
        return MediaSourceType::Unknown;
    }

private:
    std::string url_;
    MediaSourceType type_{MediaSourceType::Unknown};
    bool opened_{false};
};

} // namespace rav

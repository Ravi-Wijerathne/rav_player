#pragma once

#include <memory>
#include <string>

namespace rav {

enum class MediaSourceType {
    Unknown,
    LocalFile,
    HTTP,
    HTTPS,
    HLS,
    RTSP,
    NetworkStream
};

class MediaSource {
public:
    virtual ~MediaSource() = default;

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;

    virtual const std::string& url() const = 0;
    virtual MediaSourceType type() const = 0;
};

} // namespace rav

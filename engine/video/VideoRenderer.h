#pragma once

#include <memory>

#include "VideoFrame.h"

namespace rav {

class VideoRenderer {
public:
    virtual ~VideoRenderer() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual bool present_frame(const VideoFrame& frame) = 0;

    virtual void resize(int width, int height) = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;

    virtual bool is_ready() const = 0;
};

} // namespace rav

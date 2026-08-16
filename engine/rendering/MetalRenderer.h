#pragma once

#include <memory>

#include "../video/VideoFrame.h"

namespace rav {

class MetalRenderer {
public:
    MetalRenderer();
    ~MetalRenderer();

    MetalRenderer(const MetalRenderer&) = delete;
    MetalRenderer& operator=(const MetalRenderer&) = delete;

    MetalRenderer(MetalRenderer&&) noexcept;
    MetalRenderer& operator=(MetalRenderer&&) noexcept;

    bool init();
    void shutdown();

    bool present_frame(const VideoFrame& frame);
    void resize(int width, int height);

    int width() const;
    int height() const;
    bool is_ready() const;

    void set_layer(void* metal_layer);
    void* layer() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rav

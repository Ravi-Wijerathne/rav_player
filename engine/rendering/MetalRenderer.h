#pragma once

#if defined(__APPLE__)

#include <memory>

#include "../video/VideoFrame.h"
#include "../video/VideoRenderer.h"

namespace rav {

class MetalRenderer : public VideoRenderer {
public:
    MetalRenderer();
    ~MetalRenderer() override;

    MetalRenderer(const MetalRenderer&) = delete;
    MetalRenderer& operator=(const MetalRenderer&) = delete;

    MetalRenderer(MetalRenderer&&) noexcept;
    MetalRenderer& operator=(MetalRenderer&&) noexcept;

    bool init() override;
    void shutdown() override;

    bool present_frame(const VideoFrame& frame) override;
    void resize(int width, int height) override;

    int width() const override;
    int height() const override;
    bool is_ready() const override;

    void set_layer(void* metal_layer);
    void* layer() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rav

#endif // __APPLE__

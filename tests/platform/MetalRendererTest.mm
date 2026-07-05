#include <gtest/gtest.h>

#include "rendering/MetalRenderer.h"

using namespace rav;

TEST(MetalRendererTest, CreateAndDestroy) {
    MetalRenderer renderer;
    SUCCEED();
}

TEST(MetalRendererTest, NotReadyInitially) {
    MetalRenderer renderer;
    EXPECT_FALSE(renderer.is_ready());
}

TEST(MetalRendererTest, InitSucceeds) {
    MetalRenderer renderer;
    bool result = renderer.init();
    EXPECT_TRUE(result);
    EXPECT_TRUE(renderer.is_ready());
}

TEST(MetalRendererTest, ResizeAfterInit) {
    MetalRenderer renderer;
    ASSERT_TRUE(renderer.init());
    renderer.resize(1920, 1080);
    EXPECT_EQ(renderer.width(), 1920);
    EXPECT_EQ(renderer.height(), 1080);
}

TEST(MetalRendererTest, ResizeMultipleTimes) {
    MetalRenderer renderer;
    ASSERT_TRUE(renderer.init());
    renderer.resize(640, 480);
    EXPECT_EQ(renderer.width(), 640);
    EXPECT_EQ(renderer.height(), 480);
    renderer.resize(1280, 720);
    EXPECT_EQ(renderer.width(), 1280);
    EXPECT_EQ(renderer.height(), 720);
}

TEST(MetalRendererTest, Shutdown) {
    MetalRenderer renderer;
    ASSERT_TRUE(renderer.init());
    EXPECT_TRUE(renderer.is_ready());
    renderer.shutdown();
    EXPECT_FALSE(renderer.is_ready());
}

TEST(MetalRendererTest, MoveConstructor) {
    MetalRenderer renderer;
    ASSERT_TRUE(renderer.init());
    MetalRenderer moved = std::move(renderer);
    EXPECT_TRUE(moved.is_ready());
}

TEST(MetalRendererTest, LayerAccess) {
    MetalRenderer renderer;
    ASSERT_TRUE(renderer.init());
    EXPECT_EQ(renderer.layer(), nullptr);
}

TEST(MetalRendererTest, SetLayer) {
    MetalRenderer renderer;
    ASSERT_TRUE(renderer.init());
    void* original = renderer.layer();
    void* fake_layer = (void*)(intptr_t)0xDEAD;
    renderer.set_layer(fake_layer);
    EXPECT_EQ(renderer.layer(), fake_layer);
    renderer.set_layer(original);
    EXPECT_EQ(renderer.layer(), original);
}

sed -i '' 's/int current_texture_index_ = 0;/int current_texture_index_ = 0;\n    int current_frame_slot_ = 0;/g' engine/rendering/MetalRenderer.mm

sed -i '' 's/struct TextureBuffer {/struct TextureBuffer {\n        int plane = 0;\n        int slot = 0;/g' engine/rendering/MetalRenderer.mm

sed -i '' 's/id<MTLTexture> acquireTexture(int width, int height, MTLPixelFormat format) {/id<MTLTexture> acquireTexture(int width, int height, MTLPixelFormat format, int plane, int slot) {/g' engine/rendering/MetalRenderer.mm

sed -i '' 's/if (buf.width == width && buf.height == height && buf.format == format) {/if (buf.width == width \&\& buf.height == height \&\& buf.format == format \&\& buf.plane == plane \&\& buf.slot == slot) {/g' engine/rendering/MetalRenderer.mm

sed -i '' 's/if (texture_pool_.size() < 3) {/if (texture_pool_.size() < 30) {/g' engine/rendering/MetalRenderer.mm

sed -i '' 's/texture_pool_.push_back({texture, width, height, format});/texture_pool_.push_back({texture, width, height, format, plane, slot});/g' engine/rendering/MetalRenderer.mm

sed -i '' 's/texture_pool_\[current_texture_index_\] = {texture, width, height, format};/texture_pool_[current_texture_index_] = {texture, width, height, format, plane, slot};/g' engine/rendering/MetalRenderer.mm

sed -i '' 's/current_texture_index_ = (current_texture_index_ + 1) % 3;/current_texture_index_ = (current_texture_index_ + 1) % 30;/g' engine/rendering/MetalRenderer.mm

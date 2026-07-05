#if defined(__APPLE__)

#include "MetalRenderer.h"
#include "FrameConverter.h"
#include "ShaderTypes.h"

#include <cstdint>
#include <vector>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/QuartzCore.h>

namespace rav {

class MetalRenderer::Impl {
public:
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> commandQueue = nil;
    id<MTLLibrary> library = nil;
    id<MTLRenderPipelineState> pipelineState = nil;
    id<MTLRenderPipelineState> yuvPipelineState = nil;
    id<MTLBuffer> vertexBuffer0 = nil;
    id<MTLBuffer> vertexBuffer90 = nil;
    id<MTLBuffer> vertexBuffer180 = nil;
    id<MTLBuffer> vertexBuffer270 = nil;
    CAMetalLayer* metalLayer = nil;
    dispatch_semaphore_t inflightSemaphore = nil;

    int width = 0;
    int height = 0;
    bool ready = false;

    FrameConverter frame_converter_;

    struct TextureBuffer {
        id<MTLTexture> texture = nil;
        int width = 0;
        int height = 0;
        MTLPixelFormat format = MTLPixelFormatBGRA8Unorm;
    };

    std::vector<TextureBuffer> texture_pool_;
    int current_texture_index_ = 0;

    id<MTLTexture> acquireTexture(int width, int height, MTLPixelFormat format) {
        for (auto& buf : texture_pool_) {
            if (buf.width == width && buf.height == height && buf.format == format) {
                return buf.texture;
            }
        }

        MTLTextureDescriptor* desc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:format
            width:width height:height mipmapped:NO];
        desc.usage = MTLTextureUsageShaderRead;
        desc.storageMode = MTLStorageModeShared;
        id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
        if (!texture) return nil;

        if (texture_pool_.size() < 3) {
            texture_pool_.push_back({texture, width, height, format});
        } else {
            texture_pool_[current_texture_index_] = {texture, width, height, format};
            current_texture_index_ = (current_texture_index_ + 1) % 3;
        }
        return texture;
    }

    bool setupMetal() {
        device = MTLCreateSystemDefaultDevice();
        if (!device) {
            NSLog(@"MetalRenderer: MTLCreateSystemDefaultDevice failed");
            return false;
        }

        commandQueue = [device newCommandQueue];
        if (!commandQueue) {
            NSLog(@"MetalRenderer: newCommandQueue failed");
            return false;
        }

        inflightSemaphore = dispatch_semaphore_create(3);

        NSError* error = nil;

        // Try loading precompiled metallib from app bundle
        NSURL* libURL = [[NSBundle mainBundle] URLForResource:@"shaders"
                                                 withExtension:@"metallib"];
        if (libURL) {
            library = [device newLibraryWithURL:libURL error:&error];
            if (!library) {
                NSLog(@"MetalRenderer: failed to load metallib from %@: %@",
                      libURL, error.localizedDescription);
            }
        } else {
            NSLog(@"MetalRenderer: shaders.metallib not found in bundle");
        }

        // Fall back to default library (embedded in binary)
        if (!library) {
            library = [device newDefaultLibrary];
            if (!library) {
                NSLog(@"MetalRenderer: newDefaultLibrary also failed");
            }
        }

        if (!library) return false;

        if (!setupPipeline()) return false;
        setupVertices();

        ready = true;
        return true;
    }

    bool setupPipeline() {
        auto* vertexFn = [library newFunctionWithName:@"vertexShader"];
        auto* fragmentFn = [library newFunctionWithName:@"fragmentShader"];
        auto* yuvFragmentFn = [library newFunctionWithName:@"yuvFragmentShader"];

        if (!vertexFn) {
            NSLog(@"MetalRenderer: vertexShader function not found in library");
            return false;
        }

        if (fragmentFn) {
            MTLRenderPipelineDescriptor* desc = [MTLRenderPipelineDescriptor new];
            desc.label = @"RGBA Pipeline";
            desc.vertexFunction = vertexFn;
            desc.fragmentFunction = fragmentFn;
            desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

            NSError* error = nil;
            pipelineState = [device newRenderPipelineStateWithDescriptor:desc error:&error];
            if (!pipelineState) {
                NSLog(@"MetalRenderer: RGBA pipeline error: %@", error.localizedDescription);
            }
        } else {
            NSLog(@"MetalRenderer: fragmentShader function not found");
        }

        if (yuvFragmentFn) {
            MTLRenderPipelineDescriptor* desc = [MTLRenderPipelineDescriptor new];
            desc.label = @"YUV Pipeline";
            desc.vertexFunction = vertexFn;
            desc.fragmentFunction = yuvFragmentFn;
            desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

            NSError* error = nil;
            yuvPipelineState = [device newRenderPipelineStateWithDescriptor:desc error:&error];
            if (!yuvPipelineState) {
                NSLog(@"MetalRenderer: YUV pipeline error: %@", error.localizedDescription);
            }
        }

        return pipelineState != nil;
    }

    void setupVertices() {
        static const Vertex quad0[] = {
            { .position = { -1.0,  -1.0 }, .texcoord = { 0.0, 1.0 } },
            { .position = {  1.0,  -1.0 }, .texcoord = { 1.0, 1.0 } },
            { .position = { -1.0,   1.0 }, .texcoord = { 0.0, 0.0 } },
            { .position = {  1.0,   1.0 }, .texcoord = { 1.0, 0.0 } },
        };
        vertexBuffer0 = [device newBufferWithBytes:quad0
                                           length:sizeof(quad0)
                                          options:MTLResourceStorageModeShared];

        static const Vertex quad90[] = {
            { .position = { -1.0,  -1.0 }, .texcoord = { 1.0, 1.0 } },
            { .position = {  1.0,  -1.0 }, .texcoord = { 1.0, 0.0 } },
            { .position = { -1.0,   1.0 }, .texcoord = { 0.0, 1.0 } },
            { .position = {  1.0,   1.0 }, .texcoord = { 0.0, 0.0 } },
        };
        vertexBuffer90 = [device newBufferWithBytes:quad90
                                            length:sizeof(quad90)
                                           options:MTLResourceStorageModeShared];

        static const Vertex quad180[] = {
            { .position = { -1.0,  -1.0 }, .texcoord = { 1.0, 0.0 } },
            { .position = {  1.0,  -1.0 }, .texcoord = { 0.0, 0.0 } },
            { .position = { -1.0,   1.0 }, .texcoord = { 1.0, 1.0 } },
            { .position = {  1.0,   1.0 }, .texcoord = { 0.0, 1.0 } },
        };
        vertexBuffer180 = [device newBufferWithBytes:quad180
                                             length:sizeof(quad180)
                                            options:MTLResourceStorageModeShared];

        static const Vertex quad270[] = {
            { .position = { -1.0,  -1.0 }, .texcoord = { 0.0, 0.0 } },
            { .position = {  1.0,  -1.0 }, .texcoord = { 0.0, 1.0 } },
            { .position = { -1.0,   1.0 }, .texcoord = { 1.0, 0.0 } },
            { .position = {  1.0,   1.0 }, .texcoord = { 1.0, 1.0 } },
        };
        vertexBuffer270 = [device newBufferWithBytes:quad270
                                             length:sizeof(quad270)
                                            options:MTLResourceStorageModeShared];
    }

    void renderYUV(id<MTLTexture> yTexture, id<MTLTexture> uvTexture, int rotation) {
        if (!ready || !metalLayer || !yTexture || !uvTexture) {
            static int once = 0; if (++once == 1) NSLog(@"renderYUV: early return ready=%d layer=%p y=%p uv=%p", ready, metalLayer, yTexture, uvTexture);
            return;
        }
        id<MTLRenderPipelineState> ps = yuvPipelineState;
        if (!ps) { static int once = 0; if (++once == 1) NSLog(@"renderYUV: no pipeline"); return; }

        dispatch_semaphore_wait(inflightSemaphore, DISPATCH_TIME_FOREVER);

        @autoreleasepool {
            id<MTLCommandBuffer> cmdBuffer = [commandQueue commandBuffer];
            if (!cmdBuffer) {
                dispatch_semaphore_signal(inflightSemaphore);
                return;
            }

            __block dispatch_semaphore_t sema = inflightSemaphore;
            [cmdBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
                dispatch_semaphore_signal(sema);
            }];

            id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
            if (!drawable) {
                static int once = 0; if (++once == 1) NSLog(@"renderYUV: nextDrawable returned nil");
                [cmdBuffer commit];
                return;
            }

            MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            passDesc.colorAttachments[0].texture = drawable.texture;
            passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
            passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
            passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);

            id<MTLRenderCommandEncoder> encoder =
                [cmdBuffer renderCommandEncoderWithDescriptor:passDesc];
            if (!encoder) {
                [cmdBuffer commit];
                return;
            }

            encoder.label = @"Present YUV Frame";
            [encoder setViewport:(MTLViewport){
                0.0, 0.0, (double)width, (double)height, 0.0, 1.0
            }];

            [encoder setRenderPipelineState:ps];
            id<MTLBuffer> vBuf = vertexBuffer0;
            if (rotation == 90) vBuf = vertexBuffer90;
            else if (rotation == 180) vBuf = vertexBuffer180;
            else if (rotation == 270) vBuf = vertexBuffer270;
            [encoder setVertexBuffer:vBuf offset:0 atIndex:0];

            Uniforms uniforms;
            [encoder setFragmentBytes:&uniforms length:sizeof(uniforms)
                              atIndex:FragmentInputIndexUniforms];
            [encoder setFragmentTexture:yTexture atIndex:0];
            [encoder setFragmentTexture:uvTexture atIndex:1];

            [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                        vertexStart:0 vertexCount:4];

            [encoder endEncoding];
            [cmdBuffer presentDrawable:drawable];
            [cmdBuffer commit];
        }
    }

    void render(id<MTLTexture> texture, bool isYUV, int rotation) {
        if (!ready) {
            static int once = 0; if (++once == 1) NSLog(@"MetalRenderer: render called but not ready");
            return;
        }
        if (!metalLayer) {
            static int once = 0; if (++once == 1) NSLog(@"MetalRenderer: render called but metalLayer is nil");
            return;
        }
        if (!texture) {
            static int once = 0; if (++once == 1) NSLog(@"MetalRenderer: render called but texture is nil");
            return;
        }

        id<MTLRenderPipelineState> ps = isYUV ? yuvPipelineState : pipelineState;
        if (!ps) {
            static int once = 0; if (++once == 1) NSLog(@"MetalRenderer: render called but pipelineState is nil");
            return;
        }

        dispatch_semaphore_wait(inflightSemaphore, DISPATCH_TIME_FOREVER);

        @autoreleasepool {
            id<MTLCommandBuffer> cmdBuffer = [commandQueue commandBuffer];
            if (!cmdBuffer) {
                dispatch_semaphore_signal(inflightSemaphore);
                return;
            }

            __block dispatch_semaphore_t sema = inflightSemaphore;
            [cmdBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
                dispatch_semaphore_signal(sema);
            }];

            id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
            if (!drawable) {
                static int once = 0; if (++once == 1) NSLog(@"MetalRenderer: nextDrawable returned nil");
                [cmdBuffer commit];
                return;
            }

            MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
            passDesc.colorAttachments[0].texture = drawable.texture;
            passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
            passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
            passDesc.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);

            id<MTLRenderCommandEncoder> encoder =
                [cmdBuffer renderCommandEncoderWithDescriptor:passDesc];
            if (!encoder) {
                [cmdBuffer commit];
                return;
            }

            encoder.label = @"Present Frame";
            [encoder setViewport:(MTLViewport){
                0.0, 0.0, (double)width, (double)height, 0.0, 1.0
            }];

            [encoder setRenderPipelineState:ps];
            id<MTLBuffer> vBuf = vertexBuffer0;
            if (rotation == 90) vBuf = vertexBuffer90;
            else if (rotation == 180) vBuf = vertexBuffer180;
            else if (rotation == 270) vBuf = vertexBuffer270;
            [encoder setVertexBuffer:vBuf offset:0 atIndex:0];

            Uniforms uniforms;
            [encoder setFragmentBytes:&uniforms length:sizeof(uniforms)
                              atIndex:FragmentInputIndexUniforms];
            [encoder setFragmentTexture:texture atIndex:0];

            [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                        vertexStart:0 vertexCount:4];

            [encoder endEncoding];
            [cmdBuffer presentDrawable:drawable];
            [cmdBuffer commit];
        }
    }
};

MetalRenderer::MetalRenderer()
    : impl_(std::make_unique<Impl>()) {}

MetalRenderer::~MetalRenderer() { shutdown(); }

MetalRenderer::MetalRenderer(MetalRenderer&&) noexcept = default;
MetalRenderer& MetalRenderer::operator=(MetalRenderer&&) noexcept = default;

bool MetalRenderer::init() {
    return impl_->setupMetal();
}

void MetalRenderer::shutdown() {
    if (!impl_) return;
    impl_->frame_converter_.close();
    impl_->ready = false;
}

bool MetalRenderer::present_frame(const VideoFrame& frame) {
    if (!impl_->ready) {
        static int once = 0; if (++once == 1) NSLog(@"MetalRenderer::present_frame: not ready");
        return false;
    }

    int w = frame.width;
    int h = frame.height;
    if (w <= 0 || h <= 0) return false;

    bool isYUV = (frame.format == VideoFrameFormat::YUV420P ||
                  frame.format == VideoFrameFormat::NV12);

    static int diag = 0; if (++diag % 30 == 1)
        NSLog(@"present_frame: w=%d h=%d fmt=%d isYUV=%d",
              w, h, (int)frame.format, isYUV);

    if (isYUV && frame.frame && impl_->yuvPipelineState) {
        int width = frame.frame->width;
        int height = frame.frame->height;

        id<MTLTexture> yTex = impl_->acquireTexture(width, height, MTLPixelFormatR8Unorm);
        if (!yTex) return false;
        [yTex replaceRegion:MTLRegionMake2D(0, 0, width, height)
                mipmapLevel:0
                  withBytes:frame.frame->data[0]
                bytesPerRow:frame.frame->linesize[0]];

        int uv_w = width / 2;
        int uv_h = height / 2;
        id<MTLTexture> uvTex = impl_->acquireTexture(uv_w, uv_h, MTLPixelFormatRG8Unorm);
        if (!uvTex) return false;

        if (frame.format == VideoFrameFormat::NV12) {
            [uvTex replaceRegion:MTLRegionMake2D(0, 0, uv_w, uv_h)
                     mipmapLevel:0
                       withBytes:frame.frame->data[1]
                     bytesPerRow:frame.frame->linesize[1]];
        } else {
            // YUV420P: interleave separate U+V planes into RG8
            std::vector<uint8_t> uvInterleaved(uv_w * uv_h * 2);
            for (int row = 0; row < uv_h; ++row) {
                const uint8_t* uRow = frame.frame->data[1] + row * frame.frame->linesize[1];
                const uint8_t* vRow = frame.frame->data[2] + row * frame.frame->linesize[2];
                uint8_t* dst = uvInterleaved.data() + row * uv_w * 2;
                for (int col = 0; col < uv_w; ++col) {
                    dst[col * 2]     = uRow[col];
                    dst[col * 2 + 1] = vRow[col];
                }
            }
            [uvTex replaceRegion:MTLRegionMake2D(0, 0, uv_w, uv_h)
                     mipmapLevel:0
                       withBytes:uvInterleaved.data()
                     bytesPerRow:uv_w * 2];
        }

        impl_->renderYUV(yTex, uvTex, frame.rotation);
        return true;
    }

    // Fallback: CPU conversion via swscale
    if (!frame.frame) return false;

    if (!impl_->frame_converter_.is_open() ||
        impl_->frame_converter_.spec().src_width != w ||
        impl_->frame_converter_.spec().src_height != h ||
        impl_->frame_converter_.spec().src_format != frame.pix_fmt) {

        FrameConverterSpec spec;
        spec.src_width = w;
        spec.src_height = h;
        spec.src_format = frame.pix_fmt;
        spec.dst_width = w;
        spec.dst_height = h;
        spec.dst_format = AV_PIX_FMT_RGBA;
        if (!impl_->frame_converter_.init(spec)) return false;
    }

    if (!impl_->frame_converter_.convert_to_buffer(frame.frame.get())) return false;

    id<MTLTexture> texture = impl_->acquireTexture(w, h, MTLPixelFormatRGBA8Unorm);
    if (!texture) return false;

    MTLRegion region = MTLRegionMake2D(0, 0, w, h);
    [texture replaceRegion:region
               mipmapLevel:0
                 withBytes:impl_->frame_converter_.data()
               bytesPerRow:impl_->frame_converter_.linesize()];

    impl_->render(texture, false, frame.rotation);
    return true;
}

void MetalRenderer::resize(int width, int height) {
    impl_->width = width;
    impl_->height = height;
}

int MetalRenderer::width() const { return impl_->width; }
int MetalRenderer::height() const { return impl_->height; }
bool MetalRenderer::is_ready() const { return impl_->ready; }

void MetalRenderer::set_layer(void* metal_layer) {
    impl_->metalLayer = (__bridge CAMetalLayer*)metal_layer;
}

void* MetalRenderer::layer() const {
    return (__bridge void*)impl_->metalLayer;
}

} // namespace rav

#endif // __APPLE__

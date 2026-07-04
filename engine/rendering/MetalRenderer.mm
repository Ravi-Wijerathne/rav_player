#if defined(__APPLE__)

#include "MetalRenderer.h"
#include "ShaderTypes.h"

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
    id<MTLBuffer> vertexBuffer = nil;
    CAMetalLayer* metalLayer = nil;
    dispatch_semaphore_t inflightSemaphore = nil;

    int width = 0;
    int height = 0;
    bool ready = false;

    bool setupMetal() {
        device = MTLCreateSystemDefaultDevice();
        if (!device) return false;

        commandQueue = [device newCommandQueue];
        if (!commandQueue) return false;

        inflightSemaphore = dispatch_semaphore_create(3);

        NSError* error = nil;

        // Try loading precompiled metallib from app bundle
        NSURL* libURL = [[NSBundle mainBundle] URLForResource:@"shaders"
                                                withExtension:@"metallib"];
        if (libURL) {
            library = [device newLibraryWithURL:libURL error:&error];
        }

        // Fall back to default library (embedded in binary)
        if (!library) {
            library = [device newDefaultLibrary];
        }

        if (!library) return false;

        setupPipeline();
        setupVertices();

        ready = true;
        return true;
    }

    void setupPipeline() {
        auto* vertexFn = [library newFunctionWithName:@"vertexShader"];
        auto* fragmentFn = [library newFunctionWithName:@"fragmentShader"];
        auto* yuvFragmentFn = [library newFunctionWithName:@"yuvFragmentShader"];

        if (vertexFn && fragmentFn) {
            MTLRenderPipelineDescriptor* desc = [MTLRenderPipelineDescriptor new];
            desc.label = @"RGBA Pipeline";
            desc.vertexFunction = vertexFn;
            desc.fragmentFunction = fragmentFn;
            desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

            NSError* error = nil;
            pipelineState = [device newRenderPipelineStateWithDescriptor:desc error:&error];
        }

        if (vertexFn && yuvFragmentFn) {
            MTLRenderPipelineDescriptor* desc = [MTLRenderPipelineDescriptor new];
            desc.label = @"YUV Pipeline";
            desc.vertexFunction = vertexFn;
            desc.fragmentFunction = yuvFragmentFn;
            desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

            NSError* error = nil;
            yuvPipelineState = [device newRenderPipelineStateWithDescriptor:desc error:&error];
        }
    }

    void setupVertices() {
        static const Vertex quad[] = {
            { .position = { -1.0,  -1.0 }, .texcoord = { 0.0, 1.0 } },
            { .position = {  1.0,  -1.0 }, .texcoord = { 1.0, 1.0 } },
            { .position = { -1.0,   1.0 }, .texcoord = { 0.0, 0.0 } },
            { .position = {  1.0,   1.0 }, .texcoord = { 1.0, 0.0 } },
        };
        vertexBuffer = [device newBufferWithBytes:quad
                                          length:sizeof(quad)
                                         options:MTLResourceStorageModeShared];
    }

    void render(id<MTLTexture> texture, bool isYUV) {
        if (!ready || !metalLayer || !texture) return;

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

            auto* ps = isYUV ? yuvPipelineState : pipelineState;
            [encoder setRenderPipelineState:ps];
            [encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];

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
    frame_converter_.close();
    impl_->ready = false;
}

bool MetalRenderer::present_frame(const VideoFrame& frame) {
    if (!impl_->ready || !frame.frame) return false;

    int w = frame.width;
    int h = frame.height;
    if (w <= 0 || h <= 0) return false;

    if (!frame_converter_.is_open() ||
        frame_converter_.spec().src_width != w ||
        frame_converter_.spec().src_height != h ||
        frame_converter_.spec().src_format != frame.pix_fmt) {

        FrameConverterSpec spec;
        spec.src_width = w;
        spec.src_height = h;
        spec.src_format = frame.pix_fmt;
        spec.dst_width = w;
        spec.dst_height = h;
        spec.dst_format = AV_PIX_FMT_RGBA;
        if (!frame_converter_.init(spec)) return false;
    }

    if (!frame.frame || !frame_converter_.convert_to_buffer(frame.frame.get())) return false;

    id<MTLTexture> texture = nil;
    MTLTextureDescriptor* texDesc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
        width:w height:h mipmapped:NO];
    texDesc.usage = MTLTextureUsageShaderRead;
    texDesc.storageMode = MTLStorageModeShared;

    texture = [impl_->device newTextureWithDescriptor:texDesc];
    if (!texture) return false;

    MTLRegion region = MTLRegionMake2D(0, 0, w, h);
    [texture replaceRegion:region
               mipmapLevel:0
                 withBytes:frame_converter_.data()
               bytesPerRow:frame_converter_.linesize()];

    impl_->render(texture, false);
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

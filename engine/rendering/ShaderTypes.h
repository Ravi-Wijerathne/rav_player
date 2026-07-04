#pragma once

#include <simd/simd.h>

#ifdef __METAL_VERSION__
#define NS_ENUM(_type, _name) enum _name : _type _name; enum _name : _type _name
#define NSInteger metal::int32_t
#else
#import <Foundation/Foundation.h>
#endif

namespace rav {

struct Vertex {
    vector_float2 position;
    vector_float2 texcoord;
};

struct Uniforms {
    float brightness{1.0f};
    float contrast{1.0f};
    float saturation{1.0f};
    float padding;
};

} // namespace rav

typedef NS_ENUM(NSInteger, VertexInputIndex) {
    VertexInputIndexVertices = 0,
    VertexInputIndexUniforms = 1,
};

typedef NS_ENUM(NSInteger, FragmentInputIndex) {
    FragmentInputIndexTexture = 0,
    FragmentInputIndexUniforms = 1,
};

#include <metal_stdlib>
using namespace metal;

struct Vertex {
    float2 position;
    float2 texcoord;
};

struct Uniforms {
    float4x4 transform;
};

struct RasterizerData {
    float4 position [[position]];
    float2 texcoord;
};

enum VertexInputIndex {
    VertexInputIndexVertices = 0,
    VertexInputIndexUniforms = 1
};

enum FragmentInputIndex {
    FragmentInputIndexTexture = 0,
    FragmentInputIndexUniforms = 1
};

vertex RasterizerData
vertexShader(uint vertexID [[vertex_id]],
             constant Vertex *vertices [[buffer(VertexInputIndexVertices)]],
             constant Uniforms &uniforms [[buffer(VertexInputIndexUniforms)]]) {
    RasterizerData out;
    out.position = float4(vertices[vertexID].position, 0.0, 1.0);
    out.texcoord = vertices[vertexID].texcoord;
    return out;
}

fragment float4
fragmentShader(RasterizerData in [[stage_in]],
               texture2d<float> texture [[texture(FragmentInputIndexTexture)]],
               constant Uniforms &uniforms [[buffer(FragmentInputIndexUniforms)]]) {
    constexpr sampler sam(min_filter::linear, mag_filter::linear, mip_filter::none);
    float4 color = texture.sample(sam, in.texcoord);
    return color;
}

fragment float4
yuvFragmentShader(RasterizerData in [[stage_in]],
                  texture2d<float> y_texture [[texture(0)]],
                  texture2d<float> uv_texture [[texture(1)]],
                  constant Uniforms &uniforms [[buffer(FragmentInputIndexUniforms)]]) {
    constexpr sampler sam(min_filter::linear, mag_filter::linear, mip_filter::none);

    float y = y_texture.sample(sam, in.texcoord).r;
    float2 uv = uv_texture.sample(sam, in.texcoord).rg;

    y = 1.1643 * (y - 0.0625);
    float u = uv.x - 0.5;
    float v = uv.y - 0.5;

    float3 yuv = float3(y, u, v);
    float3 rgb = float3(
        yuv.x + 1.5958 * yuv.z,
        yuv.x - 0.39173 * yuv.y - 0.81290 * yuv.z,
        yuv.x + 2.017 * yuv.y
    );

    return float4(rgb, 1.0);
}

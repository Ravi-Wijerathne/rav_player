#include <metal_stdlib>
using namespace metal;

struct Vertex {
    float2 position;
    float2 texcoord;
};

struct Uniforms {
    float brightness;
    float contrast;
    float saturation;
    int is_hdr;
    int is_10bit;
    int is_bt2020;
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
             constant Vertex *vertices [[buffer(VertexInputIndexVertices)]]) {
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
    color.rgb = (color.rgb - 0.5) * uniforms.contrast + 0.5;
    color.rgb += uniforms.brightness;
    float luminance = dot(color.rgb, float3(0.299, 0.587, 0.114));
    color.rgb = mix(float3(luminance), color.rgb, uniforms.saturation + 1.0);
    return float4(color.rgb, 1.0);
}

float3 pq_to_linear(float3 pq) {
    float m1 = 2610.0 / 16384.0;
    float m2 = 2523.0 / 4096.0 * 128.0;
    float c1 = 3424.0 / 4096.0;
    float c2 = 2413.0 / 4096.0 * 32.0;
    float c3 = 2392.0 / 4096.0 * 32.0;
    
    float3 p = pow(max(pq, 0.0), 1.0 / m2);
    float3 num = max(p - c1, 0.0);
    float3 den = c2 - c3 * p;
    return pow(num / den, 1.0 / m1);
}

fragment float4
yuvFragmentShader(RasterizerData in [[stage_in]],
                  texture2d<float> y_texture [[texture(0)]],
                  texture2d<float> uv_texture [[texture(1)]],
                  constant Uniforms &uniforms [[buffer(FragmentInputIndexUniforms)]]) {
    constexpr sampler sam(min_filter::linear, mag_filter::linear, mip_filter::none);

    float y = y_texture.sample(sam, in.texcoord).r;
    float2 uv = uv_texture.sample(sam, in.texcoord).rg;

    if (uniforms.is_10bit != 0) {
        float y_10bit = y * 65535.0;
        float u_10bit = uv.x * 65535.0;
        float v_10bit = uv.y * 65535.0;

        y = (y_10bit - 64.0) / 876.0;
        uv.x = (u_10bit - 512.0) / 896.0;
        uv.y = (v_10bit - 512.0) / 896.0;
    } else {
        y = 1.1643 * (y - 0.0625);
        uv.x = uv.x - 0.5;
        uv.y = uv.y - 0.5;
    }

    float3 yuv = float3(y, uv.x, uv.y);
    float3 rgb;

    if (uniforms.is_bt2020 != 0) {
        // BT.2020 Non-Constant Luminance Matrix
        rgb = float3(
            yuv.x + 1.4746 * yuv.z,
            yuv.x - 0.16455 * yuv.y - 0.57135 * yuv.z,
            yuv.x + 1.8814 * yuv.y
        );
    } else {
        // BT.709 Matrix
        rgb = float3(
            yuv.x + 1.5958 * yuv.z,
            yuv.x - 0.39173 * yuv.y - 0.81290 * yuv.z,
            yuv.x + 2.017 * yuv.y
        );
    }

    if (uniforms.is_hdr != 0) {
        // Convert PQ transfer function to Linear
        rgb = pq_to_linear(rgb);
        // Scale to EDR (Extended Dynamic Range) where 1.0 = SDR white
        rgb *= 100.0;
    }

    return float4(rgb, 1.0);
}

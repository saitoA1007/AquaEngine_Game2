#include"Text.hlsli"

struct GlyphForGPU
{
    float4 rect; // left, top, right, bottom
    float4 uv;   // left, top, right, bottom
};
StructuredBuffer<GlyphForGPU> gGlyphs : register(t0);

VertexShaderOutput main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    // 矩形の4隅 (左上, 右上, 左下 / 左下, 右上, 右下)
    static const float2 kCorners[6] =
    {
        float2(0.0f, 0.0f), float2(1.0f, 0.0f), float2(0.0f, 1.0f),
        float2(0.0f, 1.0f), float2(1.0f, 0.0f), float2(1.0f, 1.0f),
    };

    GlyphForGPU glyph = gGlyphs[instanceId];
    float2 corner = kCorners[vertexId];

    VertexShaderOutput output;
    float2 position = lerp(glyph.rect.xy, glyph.rect.zw, corner);
    output.position = mul(float4(position, 0.0f, 1.0f), WVP);
    output.texcoord = lerp(glyph.uv.xy, glyph.uv.zw, corner);
    return output;
}

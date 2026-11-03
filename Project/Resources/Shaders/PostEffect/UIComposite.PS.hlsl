#include "FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint gameTextureHandle;
    uint uiTextureHandle;
    float2 pad;
};
ConstantBuffer<Material> gMaterial : register(b0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 scene = gTexture[gMaterial.gameTextureHandle].Sample(gSampler, input.texcoord);
    // UIは乗算済みアルファで描かれている
    float4 ui = gTexture[gMaterial.uiTextureHandle].Sample(gSampler, input.texcoord);

    float4 result;
    result.rgb = ui.rgb + scene.rgb * (1.0f - ui.a);
    result.a = ui.a + scene.a * (1.0f - ui.a);
    return result;
}

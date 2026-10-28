#include "FullScreen.hlsli"
#include "../ColorConvert.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint textureHandle;
    float highLumMask; // 明るさの範囲
    float2 pad;
};
ConstantBuffer<Material> gMaterial : register(b0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 textureColor = gTexture[gMaterial.textureHandle].Sample(gSampler, input.texcoord);
    float4 color = textureColor;
    
    float3 texColorHSV = RGBToHSV(textureColor.rgb);
    
    // HSV上の輝度を使って輝度が低いと描画しないようにする
    if (texColorHSV.z < gMaterial.highLumMask)
    {
        color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    return color;
}
#include"../FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float2 center;  // 中心点
    int numSamles;  // サンプリング処理。大きい程滑らか
    float blurWidth;
    uint textureHandle;
    float3 padding;
};
ConstantBuffer<Material> gMaterial : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    float2 direction = input.texcoord - gMaterial.center;
    float3 outputColor = float3(0.0f, 0.0f, 0.0f);
    for (int sampleIndex = 0; sampleIndex < gMaterial.numSamles; ++sampleIndex)
    {
        float2 texcoord = input.texcoord + direction * gMaterial.blurWidth * float(sampleIndex);
        outputColor.rgb += gTexture[gMaterial.textureHandle].Sample(gSampler, texcoord).rgb;
    }
    // 平均化
    outputColor.rgb *= rcp(gMaterial.numSamles);
    
    PixelShaderOutput output;
    output.color.rgb = outputColor;
    output.color.a = 1.0f;
    return output;
}
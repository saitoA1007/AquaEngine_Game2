#include"../FullScreen.hlsli"

Texture2D<float32_t4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float32_t2 center; // 中心点
    int32_t numSamles; // サンプリング処理。大きい程滑らか
    float32_t blurWidth;
    uint32_t textureHandle;
};
ConstantBuffer<Material> gMaterial : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    float32_t2 direction = input.texcoord - gMaterial.center;
    float32_t3 outputColor = float32_t3(0.0f, 0.0f, 0.0f);
    for (int32_t sampleIndex = 0; sampleIndex < gMaterial.numSamles; ++sampleIndex)
    {
        float32_t2 texcoord = input.texcoord + direction * gMaterial.blurWidth * float32_t(sampleIndex);
        outputColor.rgb += gTexture[gMaterial.textureHandle].Sample(gSampler, texcoord).rgb;
    }
    // 平均化
    outputColor.rgb *= rcp(gMaterial.numSamles);
    
    PixelShaderOutput output;
    output.color.rgb = outputColor;
    output.color.a = 1.0f;
    return output;
}
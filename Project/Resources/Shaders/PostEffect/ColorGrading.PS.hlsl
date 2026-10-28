#include"FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint textureHandle;
    float3 pad;
    
    uint enableGrayscale;
    uint enableSepia;
    uint enableRandom;
    uint enableVignetting;
    
    float vignettingIntensity;
    float vignettingTime;
    
    float randomIntensity;
    float randomTime;
};
ConstantBuffer<Material> gMaterial : register(b0);

float Rand2dTo1d(float2 value, float2 dotDir = float2(12.9898, 78.233))
{
    float2 smallValue = sin(value);
    float random = dot(smallValue, dotDir);
    random = frac(sin(random) * 143758.5453);
    return random;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture[gMaterial.textureHandle].Sample(gSampler, input.texcoord);
    
    float value = dot(output.color.rgb, float3(0.2125f, 0.7154f, 0.0721f));
    
    // グレースケール
    if (gMaterial.enableGrayscale != 0)
    {
        output.color.rgb = float3(value, value, value);
    }
    
    // セピア
    if (gMaterial.enableSepia != 0)
    {
        output.color.rgb = value * float3(1.0f, 74.0f / 107.0f, 43.0f / 107.0f);
    }
    
    // ランダム
    if (gMaterial.enableRandom != 0)
    {
        // 乱数生成
        float random = Rand2dTo1d(input.texcoord * gMaterial.randomTime);
        // 色
        output.color.xyz += float3(random, random, random) * gMaterial.randomIntensity;
    }
   
    // ヴィネット
    if (gMaterial.enableVignetting != 0)
    {
        // 周囲を0に、中心になるほど明るくなるように計算で調整
        float2 correct = input.texcoord * (1.0f - input.texcoord.yx);
        // correctだけで計算すると中心の最大値が0.0625で暗すぎるのでScaleで調整。この例では16倍にして1にしている
        float vignette = correct.x * correct.y * gMaterial.vignettingIntensity;
        // とりあえず0.8乗でそれっぽくした
        vignette = saturate(pow(vignette, gMaterial.vignettingTime));
        // 係数として乗算
        output.color.rgb *= vignette;
    }
    
    return output;
}
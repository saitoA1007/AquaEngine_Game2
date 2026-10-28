#include "../FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint textureHandle;
    float sd; // 標準偏差
};
ConstantBuffer<Material> gMaterial : register(b0);

float3 Blur1D(float2 uv, float2 uvStepSize, float2 direction)
{
    float sd2 = gMaterial.sd * gMaterial.sd;
    float rcpTwoSd2 = rcp(2.0f * sd2);
    
    float w[3];
    w[0] = 1.0f;
    w[1] = exp(-1.0f * rcpTwoSd2);
    w[2] = exp(-4.0f * rcpTwoSd2);
    
    float totalWeight = 0.0f;
    float3 blurredColor = float3(0.0f, 0.0f, 0.0f);
    
    [unroll]
    for (int i = -2; i <= 2; ++i)
    {
        float weight = w[abs(i)];
        totalWeight += weight;
        
        float2 offset = direction * (float(i) * uvStepSize);
        float3 fetchColor = gTexture[gMaterial.textureHandle].Sample(gSampler, uv + offset).rgb;
        
        blurredColor += fetchColor * weight;
    }
    
    return blurredColor * rcp(totalWeight);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    uint width, height;
    gTexture[gMaterial.textureHandle].GetDimensions(width, height);
    float2 uvStepSize = float2(rcp(width), rcp(height));

    // 縦方向のぼかし
    PixelShaderOutput output;
    output.color.rgb = Blur1D(input.texcoord, uvStepSize, float2(0.0f, 1.0f));
    output.color.a = 1.0f;
    return output;
}
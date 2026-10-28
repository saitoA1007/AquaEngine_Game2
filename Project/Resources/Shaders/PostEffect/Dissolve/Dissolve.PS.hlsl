#include"../FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint textureHandle;
    uint dissolveTextureHandle;
    float threshold;
    float padding;
};
ConstantBuffer<Material> gMaterial : register(b0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture[gMaterial.textureHandle].Sample(gSampler, input.texcoord);
    
    float mask = gTexture[gMaterial.dissolveTextureHandle].Sample(gSampler, input.texcoord).r;
    
    // エッジの幅
    float edgeWidth = 0.03f;
    
    float edge = smoothstep(gMaterial.threshold, gMaterial.threshold + edgeWidth, mask)
               * (1.0f - smoothstep(gMaterial.threshold + edgeWidth, gMaterial.threshold + edgeWidth * 2.0f, mask));
    // エッジの加算
    output.color.rgb += edge * float3(0.0f, 0.1f, 1.0f);
    if (mask < gMaterial.threshold)
    {
        // マスク部分は黒色
        output.color = float4(0, 0, 0, 1);

    }  
    return output;
}
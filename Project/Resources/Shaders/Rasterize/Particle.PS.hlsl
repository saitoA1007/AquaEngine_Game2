#include"Particle.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    float3 padding0;
    float4x4 uvTransform;
    float4 specularColor;
    float shininess;
    uint textureHandle;
    float metallic;
    int isActiveShadow;
    float ior;
    float roughness;
    uint normalTextureHandle;
    float padding1;
};
ConstantBuffer<Material> gMaterial : register(b0);

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture[input.textureHandle].Sample(gSampler, transformedUV.xy);
    output.color = gMaterial.color * textureColor * input.color;
    
    // 極小アルファは完全透明として処理しない
    clip(output.color.a - 0.01f);
    
    return output;
}
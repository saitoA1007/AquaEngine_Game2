#include "FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    uint blurTextureHandle;
    uint gameTextureHandle;
    float intensity;
    float pad;
};
ConstantBuffer<Material> gMaterial : register(b0);

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float4 texColor = gTexture[gMaterial.gameTextureHandle].Sample(gSampler, input.texcoord);
    float4 result = texColor;
	
    float4 bloomColor = gTexture[gMaterial.blurTextureHandle].Sample(gSampler, input.texcoord);
    result += bloomColor * gMaterial.intensity;

    return result;
}
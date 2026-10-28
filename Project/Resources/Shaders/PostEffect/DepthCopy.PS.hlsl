#include "FullScreen.hlsli"

Texture2D<float> gSrcDepth : register(t0);
SamplerState gSampler : register(s0);

float main(VertexShaderOutput input) : SV_Depth
{
    return gSrcDepth.Sample(gSampler, input.texcoord).r;
}
#include "FullScreen.hlsli"

Texture2D<float4> gRasterColor : register(t0);
Texture2D<float> gRasterDepth : register(t1);
Texture2D<float4> gRtColor : register(t2);
Texture2D<float> gRtDepth : register(t3);

SamplerState gSampler : register(s0);

float LinearizeDepth(float depth, float nearZ, float farZ)
{
    return nearZ * farZ / (farZ - depth * (farZ - nearZ));
}

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    float4 rasterColor = gRasterColor.Sample(gSampler, input.texcoord);
    float4 rtColor = gRtColor.Sample(gSampler, input.texcoord);
    float rasterDepth = gRasterDepth.Sample(gSampler, input.texcoord).r;
    float rtDepth = gRtDepth.Sample(gSampler, input.texcoord).r;
    
    // RTカラーをベースにする
  // float3 finalColor = rtColor.rgb;
  // 
  // if (rasterDepth <= rtDepth)
  // {
  //     finalColor = lerp(finalColor, rasterColor.rgb, rasterColor.a);
  // }
    
    float3 finalColor = lerp(rtColor.rgb, rasterColor.rgb, rasterColor.a);
    
    output.color = float4(finalColor, 1.0f);
    return output;
}
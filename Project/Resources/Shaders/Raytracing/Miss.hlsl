#include "Common.hlsli"

[shader("miss")]
void MainMiss(inout Payload payload)
{
    // 現在のレイをベクトルとしてキューブマップから画像を得る.
    float4 color = gBackgroundTexture.SampleLevel(
        gSampler, WorldRayDirection(), 0.0);
    payload.color = color.xyz;
    payload.depth = 1.0f;
}

[shader("miss")]
void ShadowMiss(inout ShadowPayload payload)
{
    payload.isHit = false;
}
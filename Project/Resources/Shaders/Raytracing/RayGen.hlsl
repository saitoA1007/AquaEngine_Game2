#include "Common.hlsli"

[shader("raygeneration")]
void MainRayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);

    float2 d = (launchIndex.xy + 0.5) / dims.xy * 2.0 - 1.0;
    float aspect = dims.x / dims.y;

    matrix mtxViewInv = gCamera.mtxViewInv;
    matrix mtxProjInv = gCamera.mtxProjInv;
    
    RayDesc rayDesc;
    rayDesc.Origin = mul(float4(0, 0, 0, 1), mtxViewInv).xyz;

    float4 target = mul(float4(d.x, -d.y, 1, 1), mtxProjInv);
    float3 direction = mul(float4(target.xyz / target.w, 0), mtxViewInv).xyz;
    rayDesc.Direction = normalize(direction);

    rayDesc.TMin = 0.001f;
    rayDesc.TMax = 100000;

    Payload payload;
    payload.color = float3(0, 0, 0.5);
    payload.recursive = 0;
    payload.depth = 1.0f;

    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xFF;

    TraceRay(
        gRtScene,
        flags,
        rayMask,
        0, // ray index
        1, // MultiplierForGeometryContrib
        0, // miss index
        rayDesc,
        payload);
    float3 col = payload.color;

    gOutput[launchIndex.xy] = float4(col, 1);
    gDepthOutput[launchIndex.xy] = payload.depth;
}
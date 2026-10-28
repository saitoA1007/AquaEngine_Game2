#include "WBOIT.hlsli"

struct ParticleForGPU
{
    float4x4 World;
    float4 color;
    uint textureHandle;
    float3 padding;
};
StructuredBuffer<ParticleForGPU> gParticle : register(t0);

struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
    float4x4 mtxViewInv; // ビュー逆行列
    float4x4 mtxProjInv; // プロジェクション逆行列
    float4x4 mtxView; // ビュー行列
    float4x4 mtxProj; // プロジェクション行列
};
ConstantBuffer<Camera> gCamera : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
};

PSAccumIn main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    PSAccumIn output;

    // ワールド → ビュー → クリップ空間変換
    float4 worldPos = mul(input.position, gParticle[instanceId].World);
    float4 viewPos = mul(worldPos, gCamera.mtxView);
    float4 clipPos = mul(viewPos, gCamera.mtxProj);
    
    output.position = clipPos;
    output.texcoord = input.texcoord;
    output.color = gParticle[instanceId].color;
    output.textureHandle = gParticle[instanceId].textureHandle;
    output.clipDepth = clipPos.z / clipPos.w;
    return output;
}

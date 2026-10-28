#include "Fracture.hlsli"

struct FractureForGPU
{
    float4x4 World;
    float4x4 WorldInverseTranspose;
    
    uint vertexOffset; // PackedGeometryBuffer内でのチャンクの頂点開始位置
    uint indexOffset; // PackedGeometryBuffer内でのチャンクのインデックス開始位置
    uint indexCount; // IndexCountPerInstance に相当
    uint chunkId; // シェーダー側で gParticle を引くためのID
};
StructuredBuffer<FractureForGPU> gFracture : register(t0);

struct InstanceIndexConstant
{
    uint instanceIndex;
};
ConstantBuffer<InstanceIndexConstant> gInstanceIndex : register(b0);

struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
    float4x4 mtxViewInv;
    float4x4 mtxProjInv;
};
ConstantBuffer<Camera> gCamera : register(b1);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    float4 worldPos = mul(input.position, gFracture[gInstanceIndex.instanceIndex].World);
    output.position = mul(worldPos, gCamera.vpMatrix);
    output.texcoord = input.texcoord;
    output.color = float4(1.0f,1.0f,1.0f,1.0f);
    output.normal = normalize(mul(input.normal, (float3x3) gFracture[gInstanceIndex.instanceIndex].WorldInverseTranspose));
    output.worldPosition = worldPos.xyz;
    return output;
}
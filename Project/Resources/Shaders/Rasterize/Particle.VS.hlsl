#include"Particle.hlsli"

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
};
ConstantBuffer<Camera> gCamera : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
};

VertexShaderOutput main(VertexShaderInput input, uint32_t instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    float32_t4 worldPos = mul(input.position, gParticle[instanceId].World);
    output.position = mul(worldPos, gCamera.vpMatrix);
    output.texcoord = input.texcoord;
    output.color = gParticle[instanceId].color;
    output.textureHandle = gParticle[instanceId].textureHandle;
    return output;
}
#include"ParticleCS.hlsli"

StructuredBuffer<ParticleCS> gParticles : register(t0);
ConstantBuffer<PerView> gPerView : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

float4x4 MakeTranslateMatrix(float3 translate)
{
    float4x4 result =
    {
        1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		translate.x, translate.y, translate.z, 1
    };
    return result;
}

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    ParticleCS particle = gParticles[instanceId];
    float4x4 worldMatrix = MakeTranslateMatrix(particle.translate); // worldMatrixを作る
    worldMatrix[0] *= particle.scale.x;
    worldMatrix[1] *= particle.scale.y;
    worldMatrix[2] *= particle.scale.z;
    worldMatrix[3].xyz = particle.translate;
    float4 worldPos = mul(input.position, worldMatrix);
    output.position = mul(worldPos, gPerView.viewProjection);
    output.texcoord = input.texcoord;
    output.color = particle.color;
    return output;
}
#include"Grid.hlsli"

struct TransformationMatrix
{
    float32_t4x4 World;
    float32_t4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct Camera
{
    float32_t3 worldPosition;
    float32_t4x4 vpMatrix;
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
    float32_t4 worldP = mul(input.position, gTransformationMatrix.World);
    output.position = mul(worldP, gCamera.vpMatrix);
    float32_t4 worldPos = mul(input.position, gTransformationMatrix.World);
    output.worldPos = worldPos.xyz;
    return output;
}
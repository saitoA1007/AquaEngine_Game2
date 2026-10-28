#include "Object3d.hlsli"

struct TransformationMatrix
{
    float4x4 World;
    float4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
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
    float32_t4 worldPos = mul(input.position, gTransformationMatrix.World);
    output.position = mul(worldPos, gCamera.vpMatrix);
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float32_t3x3) gTransformationMatrix.WorldInverseTranspose));
    output.worldPosition = mul(input.position, gTransformationMatrix.World).xyz;  
    return output;
}
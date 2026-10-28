struct TransformationMatrix
{
    float32_t4x4 World;
    float32_t4x4 WorldInverseTranspose;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

// ライト方向からのカメラ
struct LightCamera
{
    float32_t3 worldPosition;
    float32_t4x4 vpMatrix;
};
ConstantBuffer<LightCamera> gLightCamera : register(b1);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    float32_t4 worldPos = mul(input.position, gTransformationMatrix.World);
    output.position = mul(worldPos, gLightCamera.vpMatrix);
    return output;
}

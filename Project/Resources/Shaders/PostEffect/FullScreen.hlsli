struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

float LinearizeDepth(float z)
{
    float nearZ = 0.1;
    float farZ = 200.0;

    return (2.0 * nearZ) / (farZ + nearZ - z * (farZ - nearZ));
}
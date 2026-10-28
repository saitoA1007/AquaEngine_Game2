
cbuffer cbuff0 : register(b0)
{
    float32_t4 color;
    float32_t4x4 uvTransform;
    float32_t4x4 WVP;
    uint32_t textureHandle;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

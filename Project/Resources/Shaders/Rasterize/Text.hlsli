
cbuffer TextConstants : register(b0)
{
    float4x4 WVP;
    float4 color;
    float2 atlasSize;
    float distanceRange;
    uint textureHandle;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

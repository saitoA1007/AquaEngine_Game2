#include"Sprite.hlsli"

VertexShaderOutput main(float4 position : POSITION, float2 texcoord : TEXCOORD)
{
    VertexShaderOutput output;
    output.position = mul(position, WVP);
    output.texcoord = texcoord;
    return output;
}
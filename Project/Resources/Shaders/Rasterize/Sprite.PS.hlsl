#include"Sprite.hlsli"

Texture2D<float32_t4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

float4 main(VertexShaderOutput input) : SV_TARGET
{    
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), uvTransform);
    float32_t4 textureColor = gTexture[textureHandle].Sample(gSampler, transformedUV.xy);
    float32_t4 outputColor = textureColor * color;

    if (outputColor.a == 0.0f)
    {
        discard;
    }
    return outputColor;

}
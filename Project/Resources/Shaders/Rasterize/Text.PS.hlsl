#include"Text.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

float Median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

// 画面上で距離レンジが何ピクセルになるか
float ScreenPxRange(float2 texcoord)
{
    float2 unitRange = distanceRange / atlasSize;
    float2 screenTexSize = 1.0f / fwidth(texcoord);
    return max(0.5f * dot(unitRange, screenTexSize), 1.0f);
}

float4 main(VertexShaderOutput input) : SV_TARGET
{
    float3 msd = gTexture[textureHandle].Sample(gSampler, input.texcoord).rgb;

    // 3チャンネルの中央値が符号付き距離
    float sd = Median(msd.r, msd.g, msd.b);
    float screenPxDistance = ScreenPxRange(input.texcoord) * (sd - 0.5f);
    float opacity = saturate(screenPxDistance + 0.5f);

    float4 outputColor = float4(color.rgb, color.a * opacity);
    if (outputColor.a == 0.0f)
    {
        discard;
    }
    return outputColor;
}

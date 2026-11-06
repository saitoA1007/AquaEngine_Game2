#include "FullScreen.hlsli"

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct FogParam
{
    uint textureHandle;
    uint depthTextureHandle;
    float nearZ;
    float farZ;

    float4 color; // フォグの色

    float density; // 濃さ
    float startDistance; // フォグが掛かり始める距離
    float maxOpacity; // フォグの最大の濃さ
    float skyOpacity; // 空に掛けるフォグの濃さ
};
ConstantBuffer<FogParam> gFog : register(b0);

// 深度値からカメラからの距離に変換する
float ToViewDistance(float depth)
{
    return gFog.nearZ * gFog.farZ / (gFog.farZ - depth * (gFog.farZ - gFog.nearZ));
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gTexture[gFog.textureHandle].Sample(gSampler, input.texcoord);

    // レイトレの深度値
    float depth = gTexture[gFog.depthTextureHandle].Sample(gSampler, input.texcoord).r;

    float fogFactor = 0.0f;
    if (depth >= 0.9999f)
    {
        // 空は地平線と馴染ませる程度に掛ける
        fogFactor = gFog.skyOpacity;
    } else {
        // 距離に応じて指数関数的に濃くする
        float distance = max(ToViewDistance(depth) - gFog.startDistance, 0.0f);
        fogFactor = 1.0f - exp(-gFog.density * distance);
        fogFactor = min(fogFactor, gFog.maxOpacity);
    }

    output.color.rgb = lerp(output.color.rgb, gFog.color.rgb, fogFactor);
    return output;
}

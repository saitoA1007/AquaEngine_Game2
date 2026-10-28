#include "FullScreen.hlsli"

Texture2D<float4> gAccumTex : register(t0); // AccumulationRT
Texture2D<float> gRevealTex : register(t1); // RevealageRT
Texture2D<float4> gBackgroundTex : register(t2); // 不透明ジオメトリのカラーバッファ

SamplerState gPointSampler : register(s0);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
   
    int3 coord = int3((int2) input.position.xy, 0);

    float4 accum = gAccumTex.Load(coord);
    float revealage = gRevealTex.Load(coord);

    // 半透明フラグメントが存在しなければ背景をそのまま返す
    if (revealage > 0.9999f)
    {
        output.color = gBackgroundTex.Load(coord);
        return output;
    }

    // weightedAlphaの合計
    float weightedAlphaSum = accum.a;

    // ゼロ割り防止
    if (weightedAlphaSum < 1e-5f)
    {
        output.color = gBackgroundTex.Load(coord);
        return output;
    }

    // 平均カラー
    float3 avgColor = accum.rgb / weightedAlphaSum;

    // 全フラグメントを通過した後の透過率
    float T = revealage;

    // 最終合成
    float4 background = gBackgroundTex.Load(coord);
    float3 finalColor = lerp(background.rgb, avgColor, 1.0f - T);
    
    output.color = float4(finalColor, 1.0f);
    return output;
}

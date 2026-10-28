#include "WBOIT.hlsli"

struct WBOITConstants
{
    float nearPlane; // ニアプレーン距離
    float farPlane; // ファープレーン距離
    float alphaThreshold; // 棄却アルファ閾値
    
    float depthPow; // 深度感度指数
    float weightMin; // 重みの下限
    float weightMax; // 重みの上限
    float2 pad;
};

ConstantBuffer<WBOITConstants> gWBOITConstants : register(b0);

struct Material
{
    float4 color;
    int enableLighting;
    float3 padding0;
    float4x4 uvTransform;
    float4 specularColor;
    float shininess;
    uint textureHandle;
    float metallic;
    int isActiveShadow;
    float ior;
    float roughness;
    uint normalTextureHandle;
    float padding1;
};
ConstantBuffer<Material> gMaterial : register(b1);

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

// 正規化線形深度 [0, 1] を返す
float NormalizedLinearDepth(float clipDepth, float near, float far)
{
    float linearZ = (near * far) / (far - clipDepth * (far - near));
    return saturate((linearZ - near) / (far - near));
}

// zNorm[0,1]をそのまま使用
float ComputeWeight(float zNorm, float alpha, float depthPow, float weightMin, float weightMax)
{
    float depthTerm = pow(max(zNorm, 1e-4f), depthPow);
    float w = alpha / (1e-5f + depthTerm);
    return alpha * clamp(w, weightMin, weightMax);
}

PSAccumOut main(PSAccumIn input)
{  
    PSAccumOut output;
    
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture[input.textureHandle].Sample(gSampler, transformedUV.xy);
    float4 baseColor = gMaterial.color * textureColor * input.color;
    
    float alpha = baseColor.a;

    // 極小アルファは完全透明として処理しない
    clip(alpha - gWBOITConstants.alphaThreshold);

    // 正規化深度を使用
    float zNorm = NormalizedLinearDepth(input.clipDepth, gWBOITConstants.nearPlane, gWBOITConstants.farPlane);
    float w = ComputeWeight(zNorm, alpha, gWBOITConstants.depthPow, gWBOITConstants.weightMin, gWBOITConstants.weightMax);

    // RT0
    output.accumulation = float4(baseColor.rgb * alpha * w, alpha * w);

    // RT1
    output.revealage = alpha;
    return output;
}

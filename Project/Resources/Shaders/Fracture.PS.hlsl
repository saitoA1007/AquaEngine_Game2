#include "Fracture.hlsli"
#include "LightElement.hlsli"

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
ConstantBuffer<Material> gMaterial : register(b0);

struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
    float4x4 mtxViewInv;
    float4x4 mtxProjInv;
};
ConstantBuffer<Camera> gCamera : register(b1);

cbuffer LightGroup : register(b2)
{
    DirectionalLight gDirectionalLight;
    PointLight gPointLight;
    SpotLight gSpotLight;
    uint environmentTexture;
    int isActiveEnvironment;
};

Texture2D<float4> gTexture[] : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture[0].Sample(gSampler, transformedUV.xy);
    output.color = gMaterial.color * textureColor * input.color;
    
    float3 worldNormal = normalize(input.normal);
    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - input.worldPosition);
    float3 albedoColor = gMaterial.color.rgb * textureColor.rgb;
    
    // ライト
    float3 lightDir = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.xyz * gDirectionalLight.intensity;
    
    // 平行光源
    float3 directLight = CalculateBRDF(albedoColor, worldNormal, viewDir, lightDir, lightColor, gMaterial.roughness, gMaterial.metallic);
    
    // オブジェクトの色を取得
    output.color.rgb = directLight;
    
    // 極小アルファは完全透明として処理しない
    clip(output.color.a - 0.01f);
    
    return output;
}
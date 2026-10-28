#include "Object3d.hlsli"
#include "../LightElement.hlsli"

struct Material
{
    float4 color;
    
    int enableLighting;
    float dissolveThreshold;
    float2 padding0;
    
    float4x4 uvTransform;
    
    float4 specularColor;
    
    float shininess;
    uint textureHandle;
    float metallic;
    int isActiveShadow;
    
    float ior;
    float roughness;
    uint normalTextureHandle;
    uint dissolveTextureHandle;
};
ConstantBuffer<Material> gMaterial : register(b0);

Texture2D<float4> gTexture[] : register(t0,space0);
TextureCube<float4> gCubeTexture[] : register(t1,space1);
Texture2D<float> gShadowMap[] : register(t2,space2);
SamplerState gSampler : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
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

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// ライトの処理
float3 CalculateShading(float3 lightDirection, float3 lightColor, float3 normal, float3 viewDirection, float3 materialColor, Material matData);
float3 CalculateDirectionalLight(DirectionalLight light,float3 normal,float3 viewDirection,float3 materialColor,Material matData);
float3 CalculatePointLight(PointLight light,float3 worldPosition,float3 normal,float3 viewDirection,float3 materialColor,Material matData);
float3 CalculateSpotLight(SpotLight light, float3 worldPosition, float3 normal, float3 viewDirection, float3 materialColor, Material matData);
float3 CalculateEnvironmentMap(float3 worldPosition, float3 normal, float3 cameraPosition);
// 影を計算する
float CalculateShadow(float4 shadowCoord);

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture[gMaterial.textureHandle].Sample(gSampler, transformedUV.xy);
    
    if (textureColor.a == 0.0)
    {
        discard;
    }
    
    if (gMaterial.enableLighting)
    { // Lightingする場合
        
        // 最終的な色
        float3 finalColor = float3(0.0f, 0.0f, 0.0f);
        
        // ライト計算のための共通データを準備
        float3 normal = normalize(input.normal);
        float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
        float3 baseColor = gMaterial.color.rgb * textureColor.rgb;
        
        if (gDirectionalLight.active)
        {
            // デフォルトは影なし
            float finalShadow = 1.0f;
            
            if (gMaterial.isActiveShadow)
            {
                float4 world = float4(input.worldPosition, 1.0f);
                float4 shadowCoord = mul(world, gDirectionalLight.vpMatrix);
                // 影の計算を実行
                float shadowFactor = CalculateShadow(shadowCoord);
                
                float shadowAtten = 1.0f - 0.8f; // 影部分の明るさ
                finalShadow = shadowFactor + shadowAtten * (1.0f - shadowFactor);
            }
            
            finalColor += CalculateDirectionalLight(gDirectionalLight, normal, toEye, baseColor, gMaterial) * finalShadow;
        }
        
        if (gPointLight.active)
        {
            finalColor += CalculatePointLight(gPointLight, input.worldPosition, normal, toEye, baseColor, gMaterial);  
        }         
        
        if (gSpotLight.active)
        {
            finalColor += CalculateSpotLight(gSpotLight, input.worldPosition, normal, toEye, baseColor, gMaterial);
        }
        
        // 環境マップを適応
        if (isActiveEnvironment)
        {
            float3 reflectedVector = CalculateEnvironmentMap(input.worldPosition, normal, gCamera.worldPosition);
            float4 environmentColor = gCubeTexture[environmentTexture].Sample(gSampler, reflectedVector);
            // 環境マップの映り込みの度合いは粗さのパラメータを使用しておこなわれる。そちらの方がライティングにおいて適している。
            finalColor += environmentColor.rgb * gMaterial.roughness;
        }
        
        // 最終的な色を設定
        output.color.rgb = finalColor;
       
        // アルファ値を適応
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {   // Lighttingしない場合
        if (gMaterial.dissolveTextureHandle != 0)
        {
            float mask = gTexture[gMaterial.dissolveTextureHandle].SampleLevel(gSampler, transformedUV.xy, 0).r;
            if (mask <= gMaterial.dissolveThreshold)
            {
                discard;
            }
        }
        output.color = gMaterial.color * textureColor;
    }
    
    if (output.color.a == 0.0)
    {
        discard;
    }
    
    return output;
}

// Blinn-Phong + Half-Lambertの計算
float3 CalculateShading(
    float3 lightDirection, // ライトへの方向ベクトル
    float3 lightColor,     // ライトの色 * 強度
    float3 normal,         // 法線
    float3 viewDirection,  // カメラへの方向
    float3 materialColor,  // マテリアル色 * テクスチャ色
    Material matData)          // マテリアル構造体
{
    // Diffuse(Half-Lambert)
    float NdotL = dot(normal, lightDirection);
    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    float3 diffuse = materialColor * lightColor * cos;

    // Specular(Blinn-Phong)
    float3 halfVector = normalize(lightDirection + viewDirection);
    float NDotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NDotH), matData.shininess);
    float3 specular = lightColor * specularPow * matData.specularColor.xyz;

    return diffuse + specular;
}

// DirectionalLightの計算
float3 CalculateDirectionalLight(
    DirectionalLight light,
    float3 normal,
    float3 viewDirection,
    float3 materialColor,
    Material matData)
{
    // 平行光源なので、ライトへの方向は -direction
    float3 lightDir = normalize(-light.direction);
    float3 lightColorIntensity = light.color.rgb * light.intensity;

    return CalculateShading(lightDir, lightColorIntensity, normal, viewDirection, materialColor, matData);
}

// PointLightの計算
float3 CalculatePointLight(
    PointLight light,
    float3 worldPosition,
    float3 normal,
    float3 viewDirection,
    float3 materialColor,
    Material matData)
{
    // ポイントライトへの方向ベクトルと距離
    float3 directionToLight = light.position - worldPosition;
    float distance = length(directionToLight);
    float3 lightDir = normalize(directionToLight);
    // 距離減衰
    float factor = pow(saturate(-distance / light.radius + 1.0), light.decay);
    float3 lightColorIntensity = light.color.rgb * light.intensity * factor;

    return CalculateShading(lightDir, lightColorIntensity, normal, viewDirection, materialColor, matData);
}

// SpotLightの計算
float3 CalculateSpotLight(
    SpotLight light,
    float3 worldPosition,
    float3 normal,
    float3 viewDirection,
    float3 materialColor,
    Material matData)
{
    // スポットライト光源位置への方向
    float3 directionToLight = light.position - worldPosition;
    float distance = length(directionToLight);
    float3 lightDirOnSurface = normalize(directionToLight);
    // 角度減衰
    float cosAngle = dot(-lightDirOnSurface, normalize(light.direction));
    float falloffFactor = saturate((cosAngle - light.cosAngle) / (light.cosFalloffStart - light.cosAngle));
    // 距離減衰
    float attenuationFactor = pow(1.0f / distance, light.decay) * saturate(1.0f - distance / light.distance);
    // 最終的な強さ
    float3 lightColorIntensity = light.color.rgb * light.intensity * attenuationFactor * falloffFactor;

    float3 shadingDir = normalize(-light.direction);
    
    return CalculateShading(shadingDir, lightColorIntensity, normal, viewDirection, materialColor, matData);
}

// 環境マップの計算
float3 CalculateEnvironmentMap(float3 worldPosition, float3 normal, float3 cameraPosition)
{
    float3 cameraToPosition = normalize(worldPosition - cameraPosition);
    float3 reflectedVector = reflect(cameraToPosition, normal);
    return reflectedVector;
}

float CalculateShadow(float4 shadowCoord)
{
    // 透視投影除算
    float3 projectCoord = shadowCoord.xyz / shadowCoord.w;

    // uv座標に変換
    projectCoord.x = projectCoord.x * 0.5f + 0.5f;
    projectCoord.y = -projectCoord.y * 0.5f + 0.5f;

    // テクスチャ範囲外かチェック
    if (projectCoord.x < 0.0f || projectCoord.x > 1.0f ||
        projectCoord.y < 0.0f || projectCoord.y > 1.0f ||
        projectCoord.z < 0.0f || projectCoord.z > 1.0f)
    {
        return 1.0f;
    }

    float bias = 0.001f;
    float currentDepth = projectCoord.z - bias;
    
    // ShadowMapサンプリング
    return gShadowMap[gDirectionalLight.isDepthTexture].SampleCmpLevelZero(gShadowSampler, projectCoord.xy, currentDepth);
}
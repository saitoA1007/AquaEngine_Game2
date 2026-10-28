#include "Fracture.hlsli"
#include "LightElement.hlsli"

struct IceMaterial
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
    
    float chipScale;
    float chipStrength;
    float edgeWidth;
    float edgeStrength;

    float microScale;
    float microStrength;
    uint heightTextureHandle;
    float heightScale;
    
    float bubbleScale;
    float bubbleMaxDepth;
    float bubbleDensity;
    float bubbleJitter;
    
    float bubbleHighlight;
    float3 padding1;
};
ConstantBuffer<IceMaterial> gMaterial : register(b0);
 
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
 
// 氷のBeer-Lambert吸収係数
static const float3 ICE_ABSORPTION_COEFF_PS = float3(0.80f, 0.25f, 0.04f);

float3x3 CalcTangentFrame(float3 N, float3 worldPos, float2 uv)
{
    float3 dp1 = ddx(worldPos);
    float3 dp2 = ddy(worldPos);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);
 
    float3 dp2perp = cross(dp2, N);
    float3 dp1perp = cross(N, dp1);
 
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    float invMax = rsqrt(max(max(dot(T, T), dot(B, B)), 1e-8f));
    return float3x3(T * invMax, B * invMax, N);
}
 
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
 
    float3 worldPosition = input.worldPosition;
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);
 
    // 幾何法線。裏面が描かれた場合は視線側へ向け直す
    float3 geoNormal = normalize(input.normal);
    if (dot(geoNormal, viewDir) < 0.0f)
    {
        geoNormal = -geoNormal;
    }
 
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float2 uv = transformedUV.xy;
 
    float3x3 tbn = CalcTangentFrame(geoNormal, worldPosition, uv);
 
    // パララックスオクルージョンマッピング
    if (gMaterial.heightTextureHandle != 0)
    {
        float3 tangentViewDir = normalize(mul(tbn, viewDir));
        uv = ParallaxOcclusionMapping(
            gTexture[gMaterial.heightTextureHandle], gSampler,
            uv, tangentViewDir, gMaterial.heightScale);
    }
 
    // アルベド
    float4 textureColor = gTexture[gMaterial.textureHandle].Sample(gSampler, uv);
    float4 baseColor = gMaterial.color * textureColor * input.color;
    float3 iceColor = gMaterial.color.rgb * textureColor.rgb;
 
    // 法線
    float3 worldNormal = geoNormal;
    if (gMaterial.normalTextureHandle != 0)
    {
        float3 tangentNormal = gTexture[gMaterial.normalTextureHandle].Sample(gSampler, uv).rgb;
        tangentNormal = tangentNormal * 2.0f - 1.0f;
        worldNormal = normalize(mul(tangentNormal, tbn));
    }
 
    // 削り出しの氷らしいディテール法線
    if (gMaterial.dissolveThreshold > 0.0f)
    {
        float mask = FBMNoise(worldPosition * 2.0f, 1);
        if (mask <= gMaterial.dissolveThreshold)
        {
            worldNormal = ChiseledIceNormal(worldNormal, worldPosition,
                                            gMaterial.chipScale, gMaterial.chipStrength,
                                            gMaterial.edgeWidth, gMaterial.edgeStrength,
                                            gMaterial.microScale, gMaterial.microStrength);
        }
    }
 
    worldNormal = normalize(worldNormal);
    if (dot(worldNormal, viewDir) < 0.0f)
    {
        worldNormal = -worldNormal;
    }
 
    // 視点から表面へ向かうベクトル
    float3 incident = -viewDir;
 
    // フレネル
    float NdotV = saturate(dot(worldNormal, viewDir));
    float r0 = (1.0f - gMaterial.ior) / (1.0f + gMaterial.ior);
    r0 = r0 * r0;
    float F = saturate(r0 + (1.0f - r0) * pow(1.0f - NdotV, 5.0f));
 
    // Beer-Lambert
    float thickness = 0.5f;
    float pathLength = thickness / max(NdotV, 0.15f);
    float absorpScale = 1.0f - gMaterial.roughness * 0.6f;
    float3 transmittance = BeerLambert(ICE_ABSORPTION_COEFF_PS * absorpScale, pathLength);
 
    float avgTransmittance = dot(transmittance, float3(1.0f, 1.0f, 1.0f) / 3.0f);
    float absorbed = 1.0f - avgTransmittance;
 
    // 吸収で失われた背景の代わりに氷の体色を置く
    float3 color = iceColor * transmittance * absorbed;
 
    // 内部の気泡
    float bubbleOpacity = 0.0f;
    if (gMaterial.bubbleMaxDepth > 0.0001f)
    {
        float3 bubbleDir = refract(incident, geoNormal, 1.0f / gMaterial.ior);
        if (dot(bubbleDir, bubbleDir) < 1e-4f)
        {
            bubbleDir = incident;
        }
 
        float3 bubbleHitPos, bubbleHitNormal;
        float hitDistance;
        bool hitBubble = ParallaxBubbleMapping(
            worldPosition, -bubbleDir,
            gMaterial.bubbleScale, gMaterial.bubbleMaxDepth,
            gMaterial.bubbleJitter, gMaterial.bubbleDensity,
            bubbleHitPos, bubbleHitNormal, hitDistance);
 
        if (hitBubble)
        {
            float3 bubbleNormal = normalize(bubbleHitNormal);
 
            float bubbleNdotV = saturate(dot(bubbleNormal, viewDir));
            float3 lDir = normalize(-gDirectionalLight.direction);
            float3 halfVec = normalize(lDir + viewDir);
 
            float bubbleDiffuse = saturate(dot(bubbleNormal, lDir)) * 0.4f;
            float bubbleSpec = pow(saturate(dot(bubbleNormal, halfVec)), 48.0f);
            float bubbleRim = pow(1.0f - bubbleNdotV, 3.0f) * 0.3f;
 
            float3 bubbleColor = (bubbleDiffuse + bubbleSpec + bubbleRim) * gMaterial.bubbleHighlight;
 
            // 奥の気泡ほど淡く
            bubbleColor *= BeerLambert(float3(0.15f, 0.05f, 0.02f), hitDistance);
 
            // 表面フレネル分だけ減衰させて内部の光として加算
            float3 F0Ice = float3(0.02f, 0.02f, 0.02f);
            float surfaceFresnel = F_Schlick(NdotV, F0Ice).x;
            bubbleColor *= (1.0f - surfaceFresnel);
 
            color += bubbleColor;
 
            // 気泡は背景を遮るので、その分だけ不透明にする
            bubbleOpacity = saturate(dot(bubbleColor, float3(1.0f, 1.0f, 1.0f) / 3.0f));
        }
    }
 
    // 直接光
    float3 lightDir = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
 
    if (gMaterial.enableLighting != 0)
    {
        float3 directLight = CalculateBRDF(iceColor, worldNormal, viewDir, lightDir,
                                           lightColor, gMaterial.roughness, gMaterial.metallic);
        // 全部足すと氷が濁るので控えめに。見た目に合わせて係数を調整する
        color += directLight * 0.35f;
    }
 
    // 疑似SSS
    if (gMaterial.roughness > 0.01f)
    {
        float scatterStr = gMaterial.roughness * 0.4f;
        float3 sss = IceFakeSSS(worldNormal, lightDir, incident, lightColor, scatterStr);
        color += sss * (1.0f - F);
    }
 
    // リムライト
    float rimFactor = pow(1.0f - NdotV, 5.0f);
    color += float3(1.0f, 1.0f, 1.0f) * rimFactor * 0.5f;
 
    float alpha = 1.0f - (1.0f - F) * avgTransmittance * (1.0f - bubbleOpacity);
    alpha = saturate(alpha) * baseColor.a;
    output.color = float4(color, alpha);
    
    // 極小アルファは完全透明として処理しない
    clip(output.color.a - 0.01f);
 
    return output;
}
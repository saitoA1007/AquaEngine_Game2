#ifndef LIGHT_HLSLI
#define LIGHT_HLSLI
#include "NoiseUtils.hlsli"
static const float PI = 3.14159265359f;

// 平行光源
struct DirectionalLight
{
    float4 color; // ライトの色
    float3 direction; // ライトの向き
    float intensity; // 輝度
    int active;
    uint isDepthTexture; // 深度値を持ったテクスチャ
    float2 padding;
    float4x4 vpMatrix;
};

// ポイントライト
struct PointLight
{
    float4 color; // ライトの色
    float3 position; // ライトの位置
    float intensity; // 輝度
    int active; // 有効化
    float radius; // ライトの届く最大距離
    float decay; // 減衰率
};

// スポットライト
struct SpotLight
{
    float4 color; // ライトの色
    float3 position; // ライトの位置
    float intensity; // 輝度
    float3 direction; // ライトの方向
    float distance; // ライトの最大距離
    float decay; // 減衰率
    float cosAngle; // 減衰率
    float cosFalloffStart; // 
    int active; // 有効化
};

// Lambert拡散反射
float3 CalcDiffuse(float3 normal, float3 lightDir, float3 lightColor, float3 albedo)
{
    float NdotL = dot(normal, lightDir);
    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    float3 diffuse = albedo * lightColor * cos;
    return diffuse;
}

// Blinn-Phong鏡面反射
float3 CalcSpecular(float3 normal, float3 lightDir, float3 viewDir,
    float3 lightColor, float3 specularColor, float shininess)
{
    float3 halfVector = normalize(lightDir + viewDir);
    float NDotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NDotH), shininess);
    float3 specular = lightColor * specularPow * specularColor;
    return specular;
}

float D_GGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * d * d);
}

float G_SchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotX / (NdotX * (1.0f - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

float3 F_Schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float3 F_EnvApprox(float NdotV, float3 F0, float3 F90)
{
    return F0 + (F90 - F0) * pow(1.0f - NdotV, 5.0f);
}

float3 CalculateBRDF(
    float3 albedo,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float roughness,
    float metallic
)
{
    // ハーフベクトル
    float3 H = normalize(V + L);

    // 金属
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    // BRDF項の計算
    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    float3 F = F_Schlick(HdotV, F0);
    
    // エネルギー保存
    float3 kS = F;
    float3 kD = (1.0f - kS);
     // 金属は拡散反射を持たない
    kD *= 1.0f - metallic;

    // Cook-Torrance鏡面反射項
    float3 numerator = D * G * F;
    float denominator = max(4.0f * NdotV * NdotL, 0.0001f);
    float3 specular = numerator / denominator;
    
    return (kD * albedo + specular * PI) * lightColor * NdotL;
}

float3 CalculateIBL(float3 albedo, float3 reflectColor, float3 N, float3 V, float metallic, float roughness)
{
    float NdotV = saturate(dot(N, V));
     // 金属
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 F90 = max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0);
    
    // 視線角度でのフレネル
    float3 F_env = F_EnvApprox(NdotV, F0, F90);
    float3 kD_env = (1.0f - F_env) * (1.0f - metallic);
    
    // 反射強度
    float reflectStrength = (1.0f - roughness) * (1.0f - roughness);
    
    // 反射色
    float3 tintedReflect = reflectColor * lerp(float3(1.0f, 1.0f, 1.0f), albedo, metallic);
    
    // 簡易アンビエント
    float3 ambient = albedo * 0.03f;
    
    return kD_env * ambient + F_env * reflectStrength * tintedReflect;
}

// ノーマルマップから法線を取得
float3 GetNormalFromMap(float4 normalMapColor, float3 normal, float4 tangent)
{
    // [0,1]範囲を[-1,1]範囲にリマップ
    float3 textureNormal = normalMapColor.rgb * 2.0f - 1.0f;
    
    // 従法線の計算
    float3 binormal = normalize(cross(normal, tangent.xyz) * tangent.w);

    // 接空間からワールド空間への変換行列
    float3x3 tbn = float3x3(tangent.xyz, binormal, normal);
    
    return normalize(mul(textureNormal, tbn));
}

// 視差遮蔽マッピング
// heightMap : ハイトマップ
// uv : UV
// tangentViewDir : 視線ベクトル
// heightScale : 変位の強さ
float2 ParallaxOcclusionMapping(
    Texture2D<float4> heightMap,
    SamplerState samplerState,
    float2 uv,
    float3 tangentViewDir,
    float heightScale)
{
    if (heightScale < 0.0001f)
    {
        return uv;
    }

    // 視線が浅いほどサンプリング数を増やしてエイリアシングを抑える
    const float minLayers = 8.0f;
    const float maxLayers = 32.0f;
    float numLayers = lerp(maxLayers, minLayers, saturate(abs(tangentViewDir.z)));

    float layerDepth = 1.0f / numLayers;
    float currentLayerDepth = 0.0f;

    // 1レイヤーあたりのUVオフセット量
    float2 P = (tangentViewDir.xy / max(abs(tangentViewDir.z), 0.05f)) * heightScale;
    float2 deltaUV = P / numLayers;

    float2 currentUV = uv;
    float currentHeight = 1.0f - heightMap.SampleLevel(samplerState, currentUV, 0).r;

    // 高さマップに沿ってレイマーチングをし、視線と表面が交差する層を探す
    [loop]
    for (int i = 0; i < (int) maxLayers; ++i)
    {
        if (currentLayerDepth >= currentHeight)
        {
            break;
        }
        currentUV -= deltaUV;
        currentHeight = 1.0f - heightMap.SampleLevel(samplerState, currentUV, 0).r;
        currentLayerDepth += layerDepth;
    }

    // 交差の前後2点を線形補間し、階段状のアーティファクトを軽減する
    float2 prevUV = currentUV + deltaUV;
    float afterDepth = currentHeight - currentLayerDepth;
    float beforeHeight = 1.0f - heightMap.SampleLevel(samplerState, prevUV, 0).r;
    float beforeDepth = beforeHeight - (currentLayerDepth - layerDepth);
    float weight = afterDepth / max(afterDepth - beforeDepth, 0.0001f);

    return lerp(currentUV, prevUV, weight);
}

// Beer-Lambert法則による体積透過率
float3 BeerLambert(float3 absorptionCoeff, float distance)
{
    return exp(-absorptionCoeff * distance);
}

// 表面下散乱の近似
float3 IceFakeSSS(float3 normal, float3 lightDir, float3 incomingRayDir, float3 lightColor, float scatterStrength)
{
    // 内部散乱光
    static const float3 ICE_SCATTER_COLOR = float3(0.72f, 0.92f, 1.00f);
    
    float wrapDiffuse = saturate(dot(normal, lightDir) * 0.5f + 0.5f);
    float backlit = pow(saturate(dot(-incomingRayDir, lightDir)), 3.0f);

    float scatter = wrapDiffuse * 0.35f + backlit * 0.65f;
    return ICE_SCATTER_COLOR * lightColor * scatter * scatterStrength;
}

float3 ProceduralCrystalNormal(float3 baseNormal, float3 worldPos, float cellSize, float strength)
{
    if (strength < 0.001f)
    {
        return baseNormal;
    }

    // セルID
    float3 cellID = floor(worldPos / max(cellSize, 0.001f));

    float3 h;
    h.x = frac(sin(dot(cellID, float3(127.1f, 311.7f, 74.7f))) * 43758.5453f);
    h.y = frac(sin(dot(cellID + 1.0f, float3(269.5f, 183.3f, 246.1f))) * 43758.5453f);
    h.z = frac(sin(dot(cellID + 2.0f, float3(113.5f, 271.9f, 124.6f))) * 43758.5453f);
    h = h * 2.0f - 1.0f;
    
    h -= dot(h, baseNormal) * baseNormal;

    return normalize(baseNormal + h * strength);
}

// 氷の表面表面の法線を計算
//
// chipScale : 大きな削り跡1個のおおよそのサイズ
// chipStrength : 削り跡1個あたりの最大摂動強度。
// edgeWidth : 境界線とみなす距離のしきい値
// edgeStrength : 境界での追加摂動の強さ。
// microScale : 表面全体に乗せる微細な荒れのサイズ
// microStrength : 微細な荒れの強度。表面のざらつきを足す
float3 ChiseledIceNormal(
    float3 baseNormal,
    float3 worldPos,
    float chipScale,
    float chipStrength,
    float edgeWidth,
    float edgeStrength,
    float microScale,
    float microStrength
)
{
    if (chipStrength < 0.001f && microStrength < 0.001f)
    {
        return baseNormal;
    }   

    // Voronoiによる不規則な大きな削り跡
    float3 cellID;
    float d1, d2;
    Voronoi3D(worldPos / max(chipScale, 0.001f), 0.85f, cellID, d1, d2);

    // セルごとに摂動方向を決定し、接線平面に射影して連続性を保つ
    float3 h = Hash3D(cellID);
    h -= dot(h, baseNormal) * baseNormal;

    // セルごとの強度変化
    float perCellDepth = Hash1D(cellID * 1.37f);
    float3 chiseledN = normalize(baseNormal + h * chipStrength * perCellDepth);

    // エッジ強調
    float edgeFactor = saturate(1.0f - (d2 - d1) / max(edgeWidth, 0.001f));
    if (edgeFactor > 0.0f && edgeStrength > 0.0f)
    {
        float3 edgeH = Hash3D(cellID * 2.71f + 5.0f);
        edgeH -= dot(edgeH, baseNormal) * baseNormal;
        float3 edgeN = normalize(baseNormal + edgeH);
        chiseledN = normalize(lerp(chiseledN, edgeN, edgeFactor * edgeStrength));
    }

    // 微細な表面の荒れを重ねる
    if (microStrength > 0.001f)
    {
        chiseledN = ProceduralCrystalNormal(chiseledN, worldPos, microScale, microStrength);
    }

    return chiseledN;
}

// 視差遮蔽マッピングを応用した氷内部の気泡表現
// localRayOrigin : オブジェクト空間でのレイ開始位置
// localViewDir : オブジェクト空間の視線ベクトル
// bubbleScale : 気泡セル1個のスケール
// maxDepth : 気泡を探索する最大深度
// jitter : セル内での気泡位置のばらつき
// existProb : セルごとの気泡出現確率
// hitPos : 気泡表面のヒット位置
// hitNormal : 気泡表面の法線
bool ParallaxBubbleMapping(
    float3 localRayOrigin,
    float3 localViewDir,
    float bubbleScale,
    float maxDepth,
    float jitter,
    float existProb,
    out float3 hitPos,
    out float3 hitNormal,
    out float hitDistance)
{
    hitPos = localRayOrigin;
    hitNormal = float3(0.0f, 0.0f, 0.0f);
    hitDistance = 0.0f;

    if (bubbleScale < 0.0001f || maxDepth < 0.0001f)
    {
        return false;
    }

    // 視線とは逆へ進む
    float3 marchDir = -localViewDir;

    const int maxSteps = 16;
    float minStep = maxDepth / (float) maxSteps;

    float traveled = 0.0f;

    [loop]
    for (int i = 0; i < maxSteps; ++i)
    {
        float3 samplePos = localRayOrigin + marchDir * traveled;
        float3 cellPos = samplePos / bubbleScale;

        float3 bubbleCenter;
        float bubbleRadius;
        float dist = BubbleSDF(cellPos, jitter, existProb, bubbleCenter, bubbleRadius);

        if (dist <= 0.0f)
        {
            // 気泡内部に到達 -> 球面として法線を再構築
            hitPos = samplePos;
            float3 centerLocal = bubbleCenter * bubbleScale;
            hitNormal = normalize(samplePos - centerLocal);
            hitDistance = traveled;
            return true;
        }

        // SDF距離分だけ進める
        traveled += max(dist * bubbleScale, minStep);
        if (traveled >= maxDepth)
        {
            break;
        }
    }

    return false;
}
#endif
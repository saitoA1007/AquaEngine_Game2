#ifndef COMMON_HLSLI
#define COMMON_HLSLI
#include "../LightElement.hlsli"

struct Payload
{
    float3 color;
    int recursive;
    float depth;
};

struct ShadowPayload
{
    bool isHit;
};

struct MyAttribute
{
    float2 barys;
};

struct Camera
{
    float3 worldPosition;
    float4x4 vpMatrix;
    float4x4 mtxViewInv; // ビュー逆行列
    float4x4 mtxProjInv; // プロジェクション逆行列
};

struct BufferRef
{
    uint type; // バッファデータのタイプ
    uint MaterialIndex; // マテリアルデータの参照するハンドル
    
    uint indexHandle; // モデルのインデックス
    uint vertexHandle; // モデルの頂点
    
    uint vertexOffset;
    uint indexOffset;
    float2 pad;
};

// Global Root Signature
RWTexture2D<float4> gOutput : register(u0);
RWTexture2D<float> gDepthOutput : register(u1);
RaytracingAccelerationStructure gRtScene : register(t0, space0);
Texture2D<float4> gTexture[] : register(t0, space1);
StructuredBuffer<BufferRef> gBufferRefs : register(t0, space2);
ByteAddressBuffer gBufferData[] : register(t0, space3);
TextureCube<float4> gBackgroundTexture : register(t1, space0);
SamplerState gSampler : register(s0);

ConstantBuffer<Camera> gCamera : register(b0);
cbuffer LightGroup : register(b1)
{
    DirectionalLight gDirectionalLight;
    PointLight gPointLight;
    SpotLight gSpotLight;
    uint environmentTexture;
    int isActiveEnvironment;
};

// レイのフィルタリングに使うインスタンスマスク
#define RAY_MASK_OPAQUE 0x01 // 不透明。影レイを完全に遮る
#define RAY_MASK_ICE    0x02 // 氷などの透過物。影レイを一部だけ遮る
#define RAY_MASK_ALL    0xFF // 全てのインスタンス

// 氷が落とす影の濃さ
static const float ICE_SHADOW_DENSITY = 0.25f;

inline float3 CalcBarycentrics(float2 barys)
{
    return float3(
        1.0 - barys.x - barys.y,
        barys.x,
        barys.y);
}

inline float2 CalcHitAttribute2(float2 vertexAttribute[3], float2 barycentrics)
{
    float2 ret;
    ret = vertexAttribute[0];
    ret += barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]);
    ret += barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
    return ret;
}

float3 CalcHitAttribute3(float3 vertexAttribute[3], float2 barycentrics)
{
    float3 ret;
    ret = vertexAttribute[0];
    ret += barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]);
    ret += barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
    return ret;
}

float4 CalcHitAttribute4(float4 vertexAttribute[3], float2 barycentrics)
{
    float4 ret;
    ret = vertexAttribute[0];
    ret += barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]);
    ret += barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
    return ret;
}

// レイの再帰チェック
inline bool checkRecursiveLimit(inout Payload payload)
{
    payload.recursive++;
    if (payload.recursive >= 4)
    {
        // 背景画像を返す
        payload.color = gBackgroundTexture.SampleLevel(
            gSampler, WorldRayDirection(), 0.0).rgb;
        return true;
    }
    return false;
}

// 反射関数
float3 Reflection(float3 worldPosition, float3 worldNormal, int recursive)
{
    float3 worldRayDir = normalize(WorldRayDirection());
    float3 reflectDir = reflect(worldRayDir, normalize(worldNormal));

    RAY_FLAG flags = RAY_FLAG_NONE;
    uint rayMask = 0xFF;

    RayDesc rayDesc;
    rayDesc.Origin = worldPosition;
    rayDesc.Direction = reflectDir;
    rayDesc.TMin = 0.001f;
    rayDesc.TMax = 100000;

    Payload reflectPayload;
    reflectPayload.color = float3(0, 0, 0);
    reflectPayload.recursive = recursive;
    TraceRay(
        gRtScene,
        flags,
        rayMask,
        0, // ray index
        1, // MultiplierForGeometryContrib
        0, // miss index
        rayDesc,
        reflectPayload);
    return reflectPayload.color;
}

// 透明度表現で使用する屈折関数
float3 TranslucentRefraction(float3 worldPosition, float3 worldNormal, int recursive, float ior)
{  
    float3 worldRayDir = normalize(WorldRayDirection());

    float nr = dot(worldNormal, worldRayDir);
    float3 refracted;
    if (nr < 0)
    {
        // 表面. 空気中 -> 屈折媒質.
        float eta = 1.0 / ior;
        refracted = refract(worldRayDir, worldNormal, eta);
    }
    else
    {
        // 裏面. 屈折媒質 -> 空気中.
        float eta = ior / 1.0;
        refracted = refract(worldRayDir, -worldNormal, eta);
    }

    if (length(refracted) < 0.01)
    {
        return Reflection(worldPosition, worldNormal, recursive);
    }
    else
    {
        // 裏面をスキップ
        RAY_FLAG flags = RAY_FLAG_CULL_BACK_FACING_TRIANGLES;
        uint rayMask = 0xFF;

        RayDesc rayDesc;
        rayDesc.Origin = worldPosition;
        rayDesc.Direction = refracted;
        rayDesc.TMin = 0.001f;
        rayDesc.TMax = 100000;

        Payload refractPayload;
        refractPayload.color = float3(0, 0, 0);
        refractPayload.recursive = recursive;
        TraceRay(
            gRtScene,
            flags,
            rayMask,
            0, // ray index
            1, // MultiplierForGeometryContrib
            0, // miss index
            rayDesc,
            refractPayload);
        return refractPayload.color;
    }
}

// 影判定用のレイ
// rayMask : 遮蔽物として扱うインスタンスのマスク
bool ShootShadowRay(float3 origin, float3 direction, uint rayMask = RAY_MASK_ALL)
{
    RayDesc rayDesc;
    rayDesc.Origin = origin;
    rayDesc.Direction = direction;
    rayDesc.TMin = 0.001f;
    rayDesc.TMax = 100000;

    ShadowPayload payload;
    payload.isHit = true;

    RAY_FLAG flags = RAY_FLAG_NONE;
    //flags |= RAY_FLAG_FORCE_OPAQUE;
    flags |= RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;

    TraceRay(
        gRtScene,
        flags,
        rayMask,
        0, // ray index
        1, // MultiplierForGeometryContrib
        1, // miss index
        rayDesc,
        payload);
    return payload.isHit;
}

// 遮蔽物の種類を区別して影の透過率を求める
float ComputeShadowFactor(float3 origin, float3 direction)
{
    // 不透明な遮蔽物は光を完全に遮る
    if (ShootShadowRay(origin, direction, RAY_MASK_OPAQUE))
    {
        return 0.0f;
    }

    // 氷は光を透過するため、薄い影にとどめる
    if (ShootShadowRay(origin, direction, RAY_MASK_ICE))
    {
        return 1.0f - ICE_SHADOW_DENSITY;
    }

    return 1.0f;
}

float3 GlassBSDF(float3 worldPos, float3 worldNormal, int recursive, float ior, float roughness, float3 glassColor)
{
    float3 worldRayDir = normalize(WorldRayDirection());
    
    bool entering = dot(worldRayDir, worldNormal) < 0.0f;
    // 常に入射側を向く
    float3 N = entering ? worldNormal : -worldNormal;
    // Snell の法則
    float eta = entering ? (1.0f / ior) : ior;
    
    // フレネル反射率
    float cosTheta = saturate(dot(-worldRayDir, N));
    float r0 = (1.0f - ior) / (1.0f + ior);
    r0 = r0 * r0;
    float F = r0 + (1.0f - r0) * pow(1.0f - cosTheta, 5.0f);
    F = saturate(F);
    
    // 屈折方向の計算
    float3 refracted = refract(worldRayDir, N, eta);
 
    // 屈折レイが存在しなければ反射のみを返す
    if (length(refracted) < 0.001f)
    {
        return Reflection(worldPos, N, recursive);
    }
    
    // 屈折レイを発射
    RayDesc refractRay;
    refractRay.Origin = worldPos;
    refractRay.Direction = refracted;
    refractRay.TMin = 0.001f;
    refractRay.TMax = 100000.0f;
    
    Payload refractPayload;
    refractPayload.color = float3(0.0f, 0.0f, 0.0f);
    refractPayload.recursive = recursive;
    refractPayload.depth = 0.0f;
    TraceRay(gRtScene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, refractRay, refractPayload);
    
    // ガラス色で透過光をティント
    float3 refractColor = refractPayload.color * glassColor;
    
    // 反射レイを発射
    float3 reflectColor = Reflection(worldPos, N, recursive);
    
    return lerp(refractColor, reflectColor, F);
}

// 氷のBeer-Lambert則の吸収係数
static const float3 ICE_ABSORPTION_COEFF = float3(0.80f, 0.25f, 0.04f);

// 影の中の氷にかける色
static const float3 ICE_SHADOW_TINT = float3(0.80f, 0.87f, 1.00f);

// 氷のレンダリング
// shadowFactor : 影の中なら0、日向なら1。直接光由来の成分だけを遮るために使う
float3 IceBSDF(float3 worldPos, float3 worldNormal, int recursive, float ior, float roughness, float3 iceColor, float shadowFactor)
{
    float3 worldRayDir = normalize(WorldRayDirection());

    // 裏面確認
    bool entering = dot(worldRayDir, worldNormal) < 0.0f;
    float3 N = entering ? worldNormal : -worldNormal;
    float eta = entering ? (1.0f / ior) : ior;

    // Fresnel反射率
    float cosTheta = saturate(dot(-worldRayDir, N));
    float r0 = (1.0f - ior) / (1.0f + ior);
    r0 = r0 * r0;
    // 氷のr0はior1.31で約1.8%ぐらいでほぼ反射は移りません。ですが、現在はいったんビジュアル面を重視して上限を上げています。
    r0 = max(r0, 0.3f);
    float F = r0 + (1.0f - r0) * pow(1.0f - cosTheta, 5.0f);
    F = saturate(F);

    // Beer-Lambert
    float3 transmittance = float3(1.0f, 1.0f, 1.0f);
    if (!entering)
    {
        float dist = RayTCurrent();
        float absorpScale = 1.0f - roughness * 0.6f;
        float3 absorption = ICE_ABSORPTION_COEFF * absorpScale;
        transmittance = BeerLambert(absorption, dist);
    }

    // 屈折方向の計算
    float3 refracted = refract(worldRayDir, N, eta);
    if (length(refracted) < 0.001f)
    {
        // 全反射
        return Reflection(worldPos, N, recursive) * transmittance;
    }

    // 屈折レイ発射 
    RayDesc refractRay;
    refractRay.Origin = worldPos;
    refractRay.Direction = refracted;
    refractRay.TMin = 0.001f;
    refractRay.TMax = 100000.0f;

    Payload refractPayload;
    refractPayload.color = float3(0.0f, 0.0f, 0.0f);
    refractPayload.recursive = recursive;
    refractPayload.depth = 0.0f;
    TraceRay(gRtScene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, refractRay, refractPayload);

    // 透過光にBeer-Lambert吸収とiceColorティントを適用
    float3 refractColor = refractPayload.color * transmittance * iceColor;

    // 反射レイ発射
    float3 reflectColor = Reflection(worldPos, N, recursive);

    // 近似SSS。影の中では消える
    float3 sssContrib = float3(0.0f, 0.0f, 0.0f);
    if (entering && roughness > 0.01f && shadowFactor > 0.0f)
    {
        float3 lightDir = normalize(-gDirectionalLight.direction);
        float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
        float scatterStr = roughness * 0.4f;
        sssContrib = IceFakeSSS(N, lightDir, worldRayDir, lightColor, scatterStr);
        sssContrib *= (1.0f - F) * shadowFactor;
    }

    return lerp(refractColor, reflectColor, F) + sssContrib;
}

#endif
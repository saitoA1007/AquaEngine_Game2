#include "Common.hlsli"
#include "../LightElement.hlsli"

struct VertexData {
    float4 position;
    float2 texcoord;
    float3 normal;
    float4 tangent;
};

struct MaterialData {
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

static const uint VERTEX_STRIDE = 52;

VertexData GetHitVertex(MyAttribute attrib, uint vertexHandle, uint indexHandle)
{
    uint start = PrimitiveIndex() * 3;
    
    float3 positions[3];
    float2 texcoords[3];
    float3 normals[3];
    float4 tangents[3];

    for (int i = 0; i < 3; ++i) {
        uint index = gBufferData[indexHandle].Load<uint>((start + i) * 4);
        
        VertexData v = gBufferData[vertexHandle].Load<VertexData>(index * VERTEX_STRIDE);
        
        positions[i] = v.position.xyz;
        normals[i] = v.normal;
        texcoords[i] = v.texcoord;
        tangents[i] = v.tangent;
    }
    
    VertexData v = (VertexData) 0;
    v.position.xyz = CalcHitAttribute3(positions, attrib.barys);
    v.position.w = 1.0f;
    v.texcoord = CalcHitAttribute2(texcoords, attrib.barys);
    v.normal = CalcHitAttribute3(normals, attrib.barys);
    v.normal = normalize(v.normal);
    v.tangent = CalcHitAttribute4(tangents, attrib.barys);
    return v;
}

[shader("closesthit")]
void MainObjectCHS(inout Payload payload, MyAttribute attrib) {    
    if (checkRecursiveLimit(payload))
    {
        return;
    }
    
    // アクセスデータを取得
    uint refHandle = InstanceID();
    BufferRef ref = gBufferRefs[refHandle];
    // マテリアルデータを取得
    MaterialData material = gBufferData[ref.MaterialIndex].Load<MaterialData>(0);
    
    // 頂点データを取得する
    VertexData vtx = GetHitVertex(attrib, ref.vertexHandle, ref.indexHandle);
    // uvをトランスフォーム
    float4 transformedUV = mul(float4(vtx.texcoord, 0.0f, 1.0f), material.uvTransform);
    
    float3 localNormal = vtx.normal;
    // ノーマルマップがあれば法線に適応
    if (material.normalTextureHandle != 0)
    {
        float4 normalMapColor = gTexture[material.normalTextureHandle].SampleLevel(gSampler, transformedUV.xy, 0);
        vtx.tangent.xyz = normalize(vtx.tangent.xyz);
        localNormal = GetNormalFromMap(normalMapColor, vtx.normal, vtx.tangent);
    }
    // ワールド空間に変換
    float3 worldPosition = mul(vtx.position, ObjectToWorld4x3());
    float3x3 normalMatrix = transpose((float3x3) WorldToObject4x3());
    float3 worldNormal = mul(localNormal, normalMatrix);
    worldNormal = normalize(worldNormal);
   
    // 深度情報を書き込む
    float4 clipPos = mul(float4(worldPosition, 1.0f), gCamera.vpMatrix);
    payload.depth = clipPos.z / clipPos.w;
  
    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);
    
     // 裏面の法線を視線側に向け直す
    if (dot(worldNormal, viewDir) < 0.0f) { worldNormal = -worldNormal;}
    
    // テクスチャカラーを取得
    float4 textureColor = gTexture[material.textureHandle].SampleLevel(gSampler, transformedUV.xy, 0);
    // アルベド色を取得
    float3 albedoColor = material.color.rgb * textureColor.rgb;
    
    // ライティングをしない場合はアルベドの色を返す
    if (!material.enableLighting)
    {
        payload.color = albedoColor;
        return;
    }
    
    // ライト
    float3 lightDir = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.xyz * gDirectionalLight.intensity;
    
    // 平行光源
    float3 directLight = CalculateBRDF(albedoColor, worldNormal, viewDir, lightDir, lightColor, material.roughness, material.metallic);
    
    // 反射レイを飛ばして反射色を取得
    float3 reflectColor = Reflection(worldPosition, worldNormal, payload.recursive);
    
    // 環境光
    float3 indirectLight = CalculateIBL(albedoColor, reflectColor, worldNormal, viewDir, material.metallic, material.roughness);
    
     // 透明度の表示
    if (ref.type == 1)
    {   
        /// 屈折
        // 屈折レイを飛ばす
        float3 refractColor = TranslucentRefraction(worldPosition, worldNormal, payload.recursive, material.ior);
        // オブジェクトの色を取得
        float3 objectColor = directLight + indirectLight;
        
        payload.color = lerp(refractColor, objectColor, material.color.a);
        return;
    }
    
    // 最終的な色を設定
    payload.color = directLight + indirectLight;
    
    // 遮蔽物が不透明かをマスクで区別して影の濃さを取得する
    float shadowFactor = ComputeShadowFactor(worldPosition, lightDir);
    // 影の中であれば、影色を設定
    payload.color.xyz *= lerp(0.5f, 1.0f, shadowFactor);
}
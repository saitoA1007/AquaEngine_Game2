#include "Common.hlsli"
#include "../LightElement.hlsli"

struct VertexData
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float4 tangent;
};

struct IceMaterialData
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
    float rimIntensity;
    float rimPower;
    float1 padding1;
    
    float4 rimColor;
};

static const uint VERTEX_STRIDE = 52;

VertexData GetHitVertex(MyAttribute attrib, uint vertexHandle, uint indexHandle, uint vertexOffset, uint indexOffset)
{
    uint start = PrimitiveIndex() * 3;
    
    float3 positions[3];
    float2 texcoords[3];
    float3 normals[3];
    float4 tangents[3];

    for (int i = 0; i < 3; ++i)
    {
        uint localIndex = gBufferData[indexHandle].Load<uint>((start + i) * 4 + indexOffset * 4);
        uint index = localIndex + vertexOffset;
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
void MainIceObjectCHS(inout Payload payload, MyAttribute attrib)
{
    if (checkRecursiveLimit(payload))
    {
        return;
    }
    
    // アクセスデータを取得
    uint refHandle = InstanceID();
    BufferRef ref = gBufferRefs[refHandle];
    // マテリアルデータを取得
    IceMaterialData material = gBufferData[ref.MaterialIndex].Load<IceMaterialData>(0);
    
    // 頂点データを取得する
    VertexData vtx = GetHitVertex(attrib, ref.vertexHandle, ref.indexHandle, ref.vertexOffset, ref.indexOffset);
    // uvをトランスフォーム
    float4 transformedUV = mul(float4(vtx.texcoord, 0.0f, 1.0f), material.uvTransform);
    vtx.tangent.xyz = normalize(vtx.tangent.xyz);
    
    // ワールド空間に変換
    float3 worldPosition = mul(vtx.position, ObjectToWorld4x3());
    
    // 深度情報を書き込む
    float4 clipPos = mul(float4(worldPosition, 1.0f), gCamera.vpMatrix);
    payload.depth = clipPos.z / clipPos.w;
  
    // 視線ベクトル
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);
       
    float2 parallaxUV = transformedUV.xy;
    // 視線ベクトルをオブジェクト空間へ戻す
    float3x3 worldToObjectRot = (float3x3) WorldToObject4x3();
    float3 localViewDir = normalize(mul(viewDir, worldToObjectRot));
    // ハイトマップがあればUVをオフセットする
    if (material.heightTextureHandle != 0)
    {
        // オブジェクト空間のTBNで接空間へ変換
        float3 N = normalize(vtx.normal);
        float3 T = vtx.tangent.xyz;
        float3 B = normalize(cross(N, T) * vtx.tangent.w);
        float3x3 tbn = float3x3(T, B, N);
        float3 tangentViewDir = normalize(mul(tbn, localViewDir));
        
        parallaxUV = ParallaxOcclusionMapping(
            gTexture[material.heightTextureHandle], gSampler,
            transformedUV.xy, tangentViewDir, material.heightScale);
    }
    
    float3 bubbleColor = float3(0.0f, 0.0f, 0.0f);
    if (material.bubbleMaxDepth > 0.0001f)
    {
        // 屈折方向を計算
        float3 localNormalForRefract = vtx.normal; // 表面法線
        float3 incident = -localViewDir; // 入射方向を反転して内向きに
        float eta = 1.0f / material.ior; // 空気→氷
        float3 refractedDir = refract(incident, localNormalForRefract, eta);

        if (dot(refractedDir, refractedDir) < 0.0001f)
        {
            refractedDir = incident;
        }
        
        float3 bubbleHitPos, bubbleHitNormalLocal;
        float hitDistance;
        bool hitBubble = ParallaxBubbleMapping(
            vtx.position.xyz, -refractedDir,
            material.bubbleScale, material.bubbleMaxDepth,
            material.bubbleJitter, material.bubbleDensity,
            bubbleHitPos, bubbleHitNormalLocal, hitDistance);

        if (hitBubble)
        {
            float3x3 normalMatrix = transpose((float3x3) WorldToObject4x3());
            float3 bubbleNormalWorld = normalize(mul(normalMatrix, bubbleHitNormalLocal));

            float NdotV = saturate(dot(bubbleNormalWorld, viewDir));
            float3 lightDir = normalize(-gDirectionalLight.direction);
            float3 halfVec = normalize(lightDir + viewDir);

            float bubbleDiffuse = saturate(dot(bubbleNormalWorld, lightDir)) * 0.4f;
            float bubbleSpec = pow(saturate(dot(bubbleNormalWorld, halfVec)), 48.0f);
            float bubbleRim = pow(1.0f - NdotV, 3.0f) * 0.3f;

            bubbleColor = float3(1.0f, 1.0f, 1.0f) * (bubbleDiffuse + bubbleSpec + bubbleRim) * material.bubbleHighlight;

            // 深度に応じて減衰させ、奥にある気泡ほど淡く見せる
            float3 absorption = float3(0.15f, 0.05f, 0.02f);
            float3 depthAttenuation = BeerLambert(absorption, hitDistance);
            bubbleColor *= depthAttenuation;
        }
    }
    
    float3 localNormal = vtx.normal;
    // ノーマルマップがあれば法線に適応
    if (material.normalTextureHandle != 0)
    {
        float4 normalMapColor = gTexture[material.normalTextureHandle].SampleLevel(gSampler, parallaxUV, 0);
        localNormal = GetNormalFromMap(normalMapColor, vtx.normal, vtx.tangent);
    }
    float3 worldNormal = mul(localNormal, (float3x3) ObjectToWorld4x3());
    worldNormal = normalize(worldNormal);
    
     // 裏面の法線を視線側に向け直す
    if (dot(worldNormal, viewDir) < 0.0f)
    {
        worldNormal = -worldNormal;
    }
    
    // テクスチャカラーを取得
    float4 textureColor = gTexture[material.textureHandle].SampleLevel(gSampler, parallaxUV, 0);
    // アルベド色を取得
    float3 albedoColor = material.color.rgb * textureColor.rgb;
    
    if (material.dissolveThreshold > 0.0f)
    {
        float mask = FBMNoise(worldPosition * 2.0f, 1);
        if (mask <= material.dissolveThreshold)
        {
             // 法線を取得
            worldNormal = ChiseledIceNormal(worldNormal, worldPosition,
                                    material.chipScale, material.chipStrength,
                                    material.edgeWidth, material.edgeStrength,
                                    material.microScale, material.microStrength);
        }
    }
    
    float3 lightDir = normalize(-gDirectionalLight.direction);
    // 影を取る
    float shadowFactor = ComputeShadowFactor(worldPosition, lightDir);

    float3 iceColor = material.color.rgb * textureColor.rgb;
    payload.color = IceBSDF(
        worldPosition,
        worldNormal,
        payload.recursive,
        material.ior,
        material.roughness,
        iceColor,
        shadowFactor
    );

    // 直射日光を失った分だけ透過、反射光をわずかに寒色へ寄せ、影の位置を判別できるようにする
    payload.color *= lerp(ICE_SHADOW_TINT, float3(1.0f, 1.0f, 1.0f), shadowFactor);

    // 平行光源による鏡面ハイライト。影の中では出ない
    if (gDirectionalLight.active)
    {
        float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
        float3 iceSpecular = CalcSpecular(worldNormal, lightDir, viewDir,
            lightColor, material.specularColor.rgb, material.shininess);
        payload.color += iceSpecular * shadowFactor;
    }

    // 視線と法線の内積を取る
    float rimNdotV = saturate(dot(worldNormal, viewDir));
    float rimFactor = 1.0f - rimNdotV;
    rimFactor = pow(rimFactor, material.rimPower);
    // リムライトの最終成分
    float3 rimLight = material.rimColor.rgb * rimFactor * material.rimIntensity * gDirectionalLight.intensity;
    payload.color += rimLight * shadowFactor;

    // バブルを描画。気泡のハイライトは影の中では消える
    float3 F0Ice = float3(0.02f, 0.02f, 0.02f);
    float NdotV = saturate(dot(worldNormal, viewDir));
    float surfaceFresnel = F_Schlick(NdotV, F0Ice).x;
    float transmittance = 1.0f - surfaceFresnel;
    payload.color += bubbleColor * transmittance * shadowFactor;
}
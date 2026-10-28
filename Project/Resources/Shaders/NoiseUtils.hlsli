#ifndef NOISE_HLSLI
#define NOISE_HLSLI

// 3次元ハッシュ。-1~1の疑似乱数
float3 Hash3D(float3 p)
{
    p = float3(dot(p, float3(127.1f, 311.7f, 74.7f)),
               dot(p, float3(269.5f, 183.3f, 246.1f)),
               dot(p, float3(113.5f, 271.9f, 124.6f)));
    return frac(sin(p) * 43758.5453f) * 2.0f - 1.0f;
}

float Hash3Dto1(float3 p)
{
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.x + p.y) * p.z);
}

// 3次元ハッシュ。0~1の疑似乱数
float Hash1D(float3 p)
{
    return frac(sin(dot(p, float3(91.3f, 157.2f, 138.7f))) * 43758.5453f);
}

// 2D用ハッシュ
float Hash2Dto1(float2 p)
{
    p = frac(p * 0.1031);
    p += dot(p, p.yx + 33.33);
    return frac((p.x + p.y) * p.y);
}

// 3次元Voronoi
void Voronoi3D(float3 p, float jitter, out float3 cellID, out float minDist, out float secondDist)
{
    float3 baseCell = floor(p);
    minDist = 1e9f;
    secondDist = 1e9f;
    cellID = baseCell;

    [unroll]
    for (int x = -1; x <= 1; x++)
    {
        [unroll]
        for (int y = -1; y <= 1; y++)
        {
            [unroll]
            for (int z = -1; z <= 1; z++)
            {
                float3 neighbor = baseCell + float3(x, y, z);
                float3 jitterOffset = Hash3D(neighbor) * 0.5f * jitter;
                float3 cellPoint = neighbor + 0.5f + jitterOffset;
                float d = distance(p, cellPoint);

                if (d < minDist)
                {
                    secondDist = minDist;
                    minDist = d;
                    cellID = neighbor;
                }
                else if (d < secondDist)
                {
                    secondDist = d;
                }
            }
        }
    }
}

// 3Dバリューノイズ
float ValueNoise3D(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    float3 u = f * f * (3.0 - 2.0 * f);
    
    float n000 = Hash3Dto1(i + float3(0, 0, 0));
    float n100 = Hash3Dto1(i + float3(1, 0, 0));
    float n010 = Hash3Dto1(i + float3(0, 1, 0));
    float n110 = Hash3Dto1(i + float3(1, 1, 0));
    float n001 = Hash3Dto1(i + float3(0, 0, 1));
    float n101 = Hash3Dto1(i + float3(1, 0, 1));
    float n011 = Hash3Dto1(i + float3(0, 1, 1));
    float n111 = Hash3Dto1(i + float3(1, 1, 1));
    
    return lerp(
        lerp(lerp(n000, n100, u.x), lerp(n010, n110, u.x), u.y),
        lerp(lerp(n001, n101, u.x), lerp(n011, n111, u.x), u.y),
        u.z);
}

// 2Dバリューノイズ
float ValueNoise2D(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);
    
    float n00 = Hash2Dto1(i + float2(0, 0));
    float n10 = Hash2Dto1(i + float2(1, 0));
    float n01 = Hash2Dto1(i + float2(0, 1));
    float n11 = Hash2Dto1(i + float2(1, 1));
    
    return lerp(lerp(n00, n10, u.x), lerp(n01, n11, u.x), u.y);
}

// FBMノイズ
float FBMNoise(float3 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    float maxValue = 0.0;
    
    [loop]
    for (int i = 0; i < octaves; ++i)
    {
        value += amplitude * ValueNoise3D(p * frequency);
        maxValue += amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value / maxValue;
}

// 気泡セルの中心、半径、存在有無を疑似乱数で決定
// cellID : 整数セル座標
// jitter : セル内での中心位置のばらつき
// existProb : このセルに気泡が存在する確率
float3 BubbleCellCenter(float3 cellID, float jitter, float existProb, out float radius, out bool exists)
{
    float3 h = Hash3D(cellID);
    float existRand = Hash1D(cellID * 3.11f + 7.0f);
    exists = existRand < existProb;

    float3 center = cellID + 0.5f + h * jitter;

    float radiusRand = Hash1D(cellID * 5.73f + 2.0f);
    radius = lerp(0.08f, 0.35f, radiusRand);

    return center;
}

// posCellから最も近い気泡球までの符号付き距離を返す
float BubbleSDF(float3 posCell, float jitter, float existProb, out float3 bubbleCenter, out float bubbleRadius)
{
    float3 baseCell = floor(posCell);
    float minDist = 1e9f;
    bubbleCenter = float3(0.0f, 0.0f, 0.0f);
    bubbleRadius = 0.0f;

    [unroll]
    for (int x = -1; x <= 1; x++)
    {
        [unroll]
        for (int y = -1; y <= 1; y++)
        {
            [unroll]
            for (int z = -1; z <= 1; z++)
            {
                float3 neighbor = baseCell + float3(x, y, z);
                float radius;
                bool exists;
                float3 center = BubbleCellCenter(neighbor, jitter, existProb, radius, exists);
                if (!exists)
                {
                    continue;
                }

                float d = distance(posCell, center) - radius;
                if (d < minDist)
                {
                    minDist = d;
                    bubbleCenter = center;
                    bubbleRadius = radius;
                }
            }
        }
    }
    return minDist;
}
#endif
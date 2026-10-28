#include"Grid.hlsli"

struct Camera
{
    float32_t3 worldPosition;
    float32_t4x4 vpMatrix;
};
ConstantBuffer<Camera> gCamera : register(b1);

float32_t4 main(VertexShaderOutput input) : SV_TARGET
{  
    float32_t2 worldXZ = input.worldPos.xz;
    
    // カメラにスナップされたグリッド原点を計算
    float32_t2 snappedOrigin = floor(gCamera.worldPosition.xz / kGridSpacing) * kGridSpacing;

    // カメラスナップを考慮したグリッド座標に変換
    float32_t2 coord = (worldXZ - snappedOrigin) / kGridSpacing;

    // グリッド線の中心からの距離
    float32_t2 grid = abs(frac(coord - 0.5f) - 0.5f) / fwidth(coord);
    
    float32_t lineValue = min(grid.x, grid.y);

    // =============================
    // 原点線の設定
    // =============================

    // Z=0線に近いか
    float32_t zAxisLine = abs(worldXZ.x) / fwidth(worldXZ.x);
    float32_t zAxisIntensity = 1.0f - smoothstep(0.0f, kOriginLineWidth, zAxisLine);

    // X=0線に近いか
    float32_t xAxisLine = abs(worldXZ.y) / fwidth(worldXZ.y);
    float32_t xAxisIntensity = 1.0f - smoothstep(0.0f, kOriginLineWidth, xAxisLine);

    // 通常のグリッド線の強度
    float32_t gridIntensity = 1.0f - smoothstep(0.0f, kNormalLineWidth, lineValue);

    // =============================
    // 色の設定
    // =============================

    float32_t3 normalColor = lerp(float32_t3(0.8f, 0.8f, 0.8f), float32_t3(0.2f, 0.2f, 0.2f), gridIntensity);
    float32_t3 xAxisColor = float32_t3(1.0f, 0.0f, 0.0f);
    float32_t3 zAxisColor = float32_t3(0.0f, 0.0f, 1.0f);

    // 色をそれぞれの強度でブレンド
    float32_t3 color = normalColor;
    color = lerp(color, xAxisColor, xAxisIntensity);
    color = lerp(color, zAxisColor, zAxisIntensity);

    // アルファは最も強い線の強度を使用
    float32_t alpha = max(gridIntensity, max(xAxisIntensity, zAxisIntensity));

    // アルファ値が0なら描画スキップ
    if (alpha <= 0.0f)
    {
        discard;
    }
    
    return float32_t4(color, alpha);
}
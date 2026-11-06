#pragma once
#include <vector>
#include "Model.h"
#include "RefBuffer.h"
#include "Material.h"
#include "WorldTransform.h"
#include "RenderQueue.h"
#include "Vector3.h"

/// <summary>
/// 氷エリアの範囲を示す仮の描画。地面に置いた、つぶした球による薄い円盤と、その縁取り。
/// 見た目だけの部品で、Draw(中心, 半径, 色)の形を保てば、このクラスごと差し替えられる。
/// （半透明の円盤は縁しか描かれず、"Cylinder"モデルは上面が描かれなかったため、不透明のつぶした球にしている）
/// </summary>
class IceAreaView {
public:
    /// <summary>
    /// 円盤に使う球モデルから、塗りと縁取り用のマテリアルと参照バッファを作る。
    /// </summary>
    /// <param name="sphereModel">半径1の球モデル（"sphere.obj"）。管理クラスが所有し、このオブジェクトより長く生存する。</param>
    explicit IceAreaView(GameEngine::Model* sphereModel);

    /// <summary>
    /// 円盤と縁取りを描画キューへ登録する。
    /// </summary>
    /// <param name="renderQueue">登録先</param>
    /// <param name="center">氷エリアの中心（XZを使う）</param>
    /// <param name="radius">氷エリアの半径</param>
    /// <param name="color">塗りの色。縁取りはこの色を白へ寄せた色にする</param>
    void Draw(GameEngine::RenderQueue* renderQueue, const Vector3& center, float radius, const Vector3& color);

private:
    // 1枚分の見た目（塗り、縁取りで1つずつ）
    struct Layer {
        std::vector<GameEngine::Material> materials;
        std::vector<GameEngine::RefBuffer> refBuffers;
    };

    /// <summary>
    /// モデルのマテリアルを元に、不透明の専用マテリアルと参照バッファを作る。
    /// </summary>
    void CreateLayer(Layer& layer);

    /// <summary>
    /// 1枚の円盤を描画キューへ登録する。
    /// </summary>
    void DrawDisc(GameEngine::RenderQueue* renderQueue, Layer& layer, const Vector3& center, float radius, float bottom, const Vector3& color);

private:
    // 調整値
    float ringWidth_ = 0.3f;       // 縁取りの幅（ワールド単位）
    float ringWhiteMix_ = 0.5f;    // 縁取りの色を白へ寄せる割合

    // 定数
    static constexpr float kThickness = 0.03f; // 円盤の厚み（つぶした球の縦の半径。上面は底から厚みの2倍の高さ）

    // 参照（所有しない）
    GameEngine::Model* model_ = nullptr;

    // 描画
    GameEngine::WorldTransform worldTransform_;
    Layer ring_; // 縁取り（外側の円盤）
    Layer fill_; // 塗り（内側の円盤。縁取りより少し高く置く）
};

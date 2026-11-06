#pragma once
#include <vector>
#include "Camera.h"
#include "Model.h"
#include "Material.h"
#include "WorldTransforms.h"
#include "RenderQueue.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Application/Game/Enemy/Enemy.h"

/// <summary>
/// 凍りかけの敵の頭に出す氷ゲージの仮表示。薄い平面をカメラと同じ向きに傾けて、敵の頭の位置に並べる。
/// ラスタライズのインスタンシングで、背景と塗りを1回ずつの描画にまとめる（敵の数が増えても描画の回数は変わらない）。
/// 見た目だけの部品で、Draw(...)の形を保てば、このクラスごと差し替えられる。
/// </summary>
class EnemyGaugeView {
public:
    /// <summary>
    /// 表示の調整値。EnemyManagerが持ち、DebugParameterへ公開する。
    /// </summary>
    struct Settings {
        bool isVisible = true;                              // ゲージを表示するか
        int32_t maxCount = 128;                             // 同時に描く上限数（確保した数まで。超えるときは近い敵を優先。全員表示だとフレームのスパイクが出た）
        float width = 1.2f;                                 // バーの幅（ワールド単位）
        float height = 0.16f;                               // バーの高さ（ワールド単位）
        float border = 0.03f;                               // 背景の余白（ワールド単位）
        float headOffset = 0.4f;                            // 敵の頭の上に空ける高さ（ワールド単位）
        Vector4 backColor = { 0.02f, 0.03f, 0.05f, 0.8f };   // 背景の色
        Vector4 fillColor = { 0.1f, 0.5f, 1.0f, 1.0f };     // 氷ゲージの色
    };

    /// <summary>
    /// 平面モデルと白いテクスチャから、描画に使う材質とインスタンスの領域を確保する。
    /// </summary>
    /// <param name="camera">バーの向きを合わせるカメラ。シーンが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="planeModel">一辺2の平面モデル（"plane.obj"。XY平面で-Z向き）。管理クラスが所有する。</param>
    /// <param name="whiteTextureHandle">単色の描画に使う白いテクスチャのハンドル</param>
    /// <param name="capacity">同時に描ける最大数（敵の最大数に合わせる）</param>
    EnemyGaugeView(const GameEngine::Camera* camera, GameEngine::Model* planeModel, uint32_t whiteTextureHandle, uint32_t capacity);

    /// <summary>
    /// 凍りかけ（氷ゲージが0より大きく、まだ凍っていない）の敵のゲージを描画キューへ登録する。
    /// 上限を超えるときは、基準位置に近い敵だけを描く。
    /// </summary>
    /// <param name="renderQueue">登録先</param>
    /// <param name="settings">表示の調整値</param>
    /// <param name="enemies">使用中の敵</param>
    /// <param name="nearCenter">上限を超えたときに近さを測る基準位置（プレイヤー位置）</param>
    void Draw(GameEngine::RenderQueue* renderQueue, const Settings& settings, const std::vector<Enemy*>& enemies, const Vector3& nearCenter);

private:
    // 定数
    static constexpr float kFillLift = 0.02f; // 塗りを背景よりカメラ側へ浮かせる量（重なりのちらつきを避ける）

    // 参照（所有しない）
    const GameEngine::Camera* camera_ = nullptr;
    GameEngine::Model* model_ = nullptr;
    uint32_t whiteTextureHandle_ = 0;

    // 状態
    uint32_t capacity_ = 0;            // 確保したインスタンス数
    std::vector<Enemy*> candidates_;   // 表示対象の敵（使い回す）

    // 描画
    GameEngine::Material material_;                 // 全バー共通の白い材質（色はインスタンスごとに持つ）
    GameEngine::WorldTransforms backTransforms_;    // 背景のインスタンス
    GameEngine::WorldTransforms fillTransforms_;    // 塗りのインスタンス
};

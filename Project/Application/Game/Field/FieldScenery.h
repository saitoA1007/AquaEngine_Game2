#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "Vector2.h"
#include "Vector4.h"
#include "Application/Game/Field/Field.h"

/// <summary>
/// フィールドの外の見た目。円の外の雪原と、台地の外に並ぶ六角柱の氷柱を描く。
/// 見た目だけで、当たり判定は持たない。
/// </summary>
class FieldScenery : public GameEngine::IGameObject {
public:
    // 氷柱の最大本数。ModelComponentは最初にこの数だけ作り、遊んでいる途中で破棄しない
    static constexpr uint32_t kMaxPillars = 64;

    /// <summary>
    /// 氷柱と雪原のモデルを受け取り、調整値を登録する。
    /// </summary>
    /// <param name="field">台地の大きさを読むフィールド。管理クラスが所有し、シーンが終わるまで有効。</param>
    /// <param name="pillarModel">登録済みの"IcePillar"モデル（上が細い六角柱、高さ1、中心は原点）</param>
    /// <param name="planeModel">planeXZ.obj（一辺2、中心は原点のXZ平面）</param>
    FieldScenery(const Field* field, GameEngine::Model* pillarModel, GameEngine::Model* planeModel);
    ~FieldScenery() override = default;

    void Initialize() override;

    /// <summary>
    /// 調整値の反映と、必要なときだけ氷柱の並べ直しを行う。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 雪原と氷柱を描画キューへ登録する。
    /// </summary>
    void Draw() override;

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 氷柱の位置・傾き・大きさを、シードから作り直す。
    /// </summary>
    void Rebuild();

    /// <summary>
    /// 雪原の大きさと位置を、台地の大きさに合わせて更新する。
    /// </summary>
    void UpdateSnowField();

    /// <summary>
    /// 氷柱の色と質感を反映する。
    /// </summary>
    void ApplyPillarMaterial();

private:
    // 参照（フィールドはシーンが終わるまで有効）
    const Field* field_ = nullptr;

    // 調整値（氷柱）
    uint32_t pillarCount_ = 40;                // 描く本数
    uint32_t seed_ = 2026;                     // 乱数のシード
    float bandInner_ = 2.0f;                   // 縁の外側から最初の氷柱までの距離
    float bandWidth_ = 6.0f;                   // 氷柱を置く帯の幅
    Vector2 heightRange_ = { 1.5f, 6.0f };     // 台地の上に出る高さの最小と最大
    Vector2 thicknessRange_ = { 0.6f, 1.5f };  // 太さの倍率の最小と最大
    float maxTiltDegree_ = 15.0f;              // 外向きに傾ける最大の角度（度）
    float angleJitter_ = 0.6f;                 // 角度のずらし（間隔に対する割合）
    Vector4 pillarColor_ = { 0.55f, 0.75f, 0.95f, 1.0f };
    float pillarRoughness_ = 0.15f;
    float pillarMetallic_ = 0.1f;

    // 調整値（雪原）
    float snowScale_ = 6.0f;                   // 台地の半径に対する雪原の広さの倍率
    Vector4 snowColor_ = { 0.18f, 0.22f, 0.3f, 1.0f };
    float snowRoughness_ = 0.95f;
    float snowMetallic_ = 0.0f;

    // 前回の台地の大きさ（変わったときだけ並べ直す）
    float lastRadius_ = -1.0f;
    float lastGroundThickness_ = -1.0f;
    float lastRimWidth_ = -1.0f;

    // 描画
    std::array<std::unique_ptr<GameEngine::ModelComponent>, kMaxPillars> pillars_;
    GameEngine::ModelComponent snowField_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

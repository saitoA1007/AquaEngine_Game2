#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include "IGameObject.h"
#include "ModelComponent.h"
#include "IceMaterial.h"
#include "DebugParameter.h"
#include "Vector2.h"
#include "Vector4.h"
#include "Application/Game/Field/Field.h"

/// <summary>
/// フィールドの外の見た目。円の外の平らな雪原と、壁の外に群れで並ぶ氷の結晶を描く。
/// 見た目だけで、当たり判定は持たない。
/// </summary>
class FieldScenery : public GameEngine::IGameObject {
public:
    // 結晶の種類の数（iceCrystalA〜D）
    static constexpr uint32_t kCrystalKinds = 4;
    // 群れの最大数と、1つの群れの結晶の最大数
    static constexpr uint32_t kMaxClusters = 16;
    static constexpr uint32_t kMaxCrystalsPerCluster = 6;
    // 種類ごとの結晶の上限。ModelComponentは最初にこの数だけ作り、遊んでいる途中で破棄しない
    static constexpr uint32_t kMaxCrystalsPerKind = 48;

    /// <summary>
    /// 結晶と雪原のモデルを受け取り、調整値を登録する。
    /// </summary>
    /// <param name="field">半径と壁の幅を読むフィールド。管理クラスが所有し、シーンが終わるまで有効。</param>
    /// <param name="crystalModels">iceCrystalA〜Dのモデル（底の中心が原点、高さ約1）。管理クラスが所有する。</param>
    /// <param name="planeModel">planeXZ.obj（一辺2、中心は原点のXZ平面）</param>
    /// <param name="snowAlbedoTexture">雪の色の画像のハンドル（0でない）</param>
    /// <param name="snowNormalTexture">雪のノーマルマップのハンドル（0でない）</param>
    FieldScenery(const Field* field, const std::array<GameEngine::Model*, kCrystalKinds>& crystalModels, GameEngine::Model* planeModel,
        uint32_t snowAlbedoTexture, uint32_t snowNormalTexture);
    ~FieldScenery() override = default;

    void Initialize() override;

    /// <summary>
    /// 調整値の反映と、必要なときだけ結晶の並べ直しを行う。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 雪原と結晶を描画キューへ登録する。
    /// </summary>
    void Draw() override;

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 結晶の氷マテリアルの調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterCrystalIceParameters();

    /// <summary>
    /// 全部の結晶の全メッシュへ、氷マテリアルと氷用のヒットグループを設定する。
    /// </summary>
    /// <param name="crystalModels">結晶のモデル（メッシュ数の取得に使う）</param>
    void SetUpCrystalIce(const std::array<GameEngine::Model*, kCrystalKinds>& crystalModels);

    /// <summary>
    /// 結晶の位置・向き・傾き・大きさを、シードから作り直す。
    /// </summary>
    void Rebuild();

    /// <summary>
    /// 雪原の大きさと位置を、フィールドの大きさに合わせて更新する。
    /// </summary>
    void UpdateSnowField();

private:
    // 参照（フィールドはシーンが終わるまで有効）
    const Field* field_ = nullptr;

    // 調整値（結晶の並べ方）
    uint32_t clusterCount_ = 10;                    // 群れの数
    Vector2 crystalsPerCluster_ = { 2.0f, 5.0f };   // 1つの群れの結晶の数の最小と最大
    uint32_t seed_ = 2026;                          // 乱数のシード
    float bandInner_ = 1.5f;                        // 壁の外側から最初の結晶までの距離
    float bandWidth_ = 6.5f;                        // 結晶を置く帯の幅
    float clusterSpread_ = 3.0f;                    // 群れの中心からの横のずれの最大
    float angleJitter_ = 0.6f;                      // 群れの角度のずらし（間隔に対する割合）
    Vector2 heightRange_ = { 1.5f, 6.0f };          // 結晶の高さの最小と最大
    float thicknessScale_ = 1.0f;                   // 結晶の太さの倍率（高さに対する）
    float maxTiltDegree_ = 15.0f;                   // 外向きに傾ける最大の角度（度）
    float occlusionRatio_ = 1.5f;                   // 高さの上限（壁の外側からの距離に対する倍率）。自機が隠れないようにする

    // 結晶の氷マテリアル（全部の結晶で共有する。値はCrystalIceの調整値）
    GameEngine::IceMaterial iceMaterial_;

    // 調整値（雪原）
    float snowScale_ = 4.0f;                        // フィールドの半径に対する雪原の広さの倍率
    float snowOffsetY_ = -0.01f;                    // 雪原の高さ（床とちらつかないよう少し下）
    Vector4 snowColor_ = { 0.45f, 0.52f, 0.65f, 1.0f };
    float snowRoughness_ = 0.9f;
    float snowMetallic_ = 0.0f;
    float snowTileSize_ = 8.0f;                     // 雪の画像1枚が覆う、雪原の上の長さ（模様の細かさ）

    // 前回のフィールドの大きさ（変わったときだけ並べ直す）
    float lastRadius_ = -1.0f;
    float lastWallWidth_ = -1.0f;

    // 描画。種類ごとに上限の数を作っておき、使う数だけ描く
    std::array<std::array<std::unique_ptr<GameEngine::ModelComponent>, kMaxCrystalsPerKind>, kCrystalKinds> crystals_;
    std::array<uint32_t, kCrystalKinds> usedCounts_ = {};
    GameEngine::ModelComponent snowField_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include "IGameObject.h"
#include "ModelComponent.h"
#include "IceMaterial.h"
#include "DebugParameter.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

/// <summary>
/// 円形のフィールド。範囲と円内への押し戻し、床（凍った湖）、境目に円周へ並ぶ氷の塊を担当する。
/// 中心はワールド原点（XZ平面）。
/// </summary>
class Field : public GameEngine::IGameObject {
public:
    // 氷の塊の種類の数（iceRockA〜C）
    static constexpr uint32_t kRockKinds = 3;
    // 氷の塊の最大の個数。ModelComponentは最初にこの数だけ作り、遊んでいる途中で破棄しない
    static constexpr uint32_t kMaxRocks = 256;

    /// <summary>
    /// 床と氷の塊のモデルを受け取り、調整値を登録する。
    /// </summary>
    /// <param name="discModel">fieldDisc.gltf（半径1の円盤、XZ平面で法線+Y、UVは真上からの投影）。管理クラスが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="rockModels">iceRockA〜Cのモデル（底の中心が原点、高さ1、平面の広がり1）。管理クラスが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="iceAlbedoTexture">ひび割れの氷の色の画像のハンドル（0でない）</param>
    /// <param name="iceNormalTexture">ひび割れの氷のノーマルマップのハンドル（0でない）</param>
    Field(GameEngine::Model* discModel, const std::array<GameEngine::Model*, kRockKinds>& rockModels, uint32_t iceAlbedoTexture, uint32_t iceNormalTexture);
    ~Field() override = default;

    void Initialize() override;

    /// <summary>
    /// 調整値の反映と、値が変わったときだけ氷の塊の並べ直しを行う。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 床と氷の塊を描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 位置を円内へ押し戻す。外へ出る分だけ円周に沿って滑る。
    /// </summary>
    /// <param name="position">補正前の位置</param>
    /// <param name="margin">端から内側へ空ける距離（対象の半径）</param>
    /// <returns>円内に収めた位置（Y座標は変えない）</returns>
    Vector3 ClampToField(const Vector3& position, float margin) const;

    /// <summary>
    /// フィールドの半径を取得する。
    /// </summary>
    float GetRadius() const { return radius_; }

    /// <summary>
    /// 境目の幅（円周から外側へ張り出す最大の距離）を取得する。
    /// </summary>
    float GetWallWidth() const;

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 氷の塊の氷マテリアルの調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterRockIceParameters();

    /// <summary>
    /// 氷の塊1つの全メッシュへ、氷マテリアルと氷用のヒットグループを設定する。
    /// </summary>
    /// <param name="rock">設定する氷の塊</param>
    /// <param name="model">氷の塊が使うモデル（メッシュ数の取得に使う）</param>
    void SetUpRockIce(GameEngine::ModelComponent& rock, const GameEngine::Model* model);

    /// <summary>
    /// 床の大きさと色、ひび割れの繰り返しを反映する。
    /// </summary>
    void UpdateFloor();

    /// <summary>
    /// 氷の塊を円周に沿って、シードから並べ直す。値が変わったときだけ呼ぶ。
    /// </summary>
    void RebuildRocks();

    /// <summary>
    /// 実際に並べる氷の塊の個数（上限内）を取得する。
    /// </summary>
    uint32_t GetRockCount() const;

private:
    // 調整値（形）
    float radius_ = 50.0f;               // フィールドの半径
    uint32_t rockCount_ = 160;           // 氷の塊の個数
    uint32_t rockSeed_ = 7;              // 並べ方の乱数のシード
    Vector2 rockHeightRange_ = { 1.0f, 1.5f };  // 氷の塊の高さの最小と最大
    Vector2 rockWidthRange_ = { 1.5f, 2.0f };   // 氷の塊の幅（平面の広がり）の最小と最大
    float rockRadialJitter_ = 0.3f;      // 円周からの半径方向のずれの最大
    float rockMaxTiltDegree_ = 12.0f;    // 傾ける最大の角度（度）
    float rockEmbedRatio_ = 0.08f;       // 底を床へ埋める深さ（高さに対する割合。縁が浮かないようにする）
    float rockInset_ = 0.2f;             // 円の内側へ食い込ませる距離（床との継ぎ目を隠す）

    // 床の色と質感（凍った湖。粗さを下げて空を映す）。画像がすでに氷の青なので、色は薄く掛ける
    Vector4 floorColor_ = { 0.9f, 0.95f, 1.0f, 1.0f };
    float floorRoughness_ = 0.08f;
    float floorMetallic_ = 0.35f;
    float crackTileSize_ = 10.0f;        // ひび割れの画像1枚が覆う、床の上の長さ（ひびの細かさ）

    // 氷の塊の氷マテリアル（全部の氷の塊で共有する。値はRockIceの調整値）
    GameEngine::IceMaterial iceMaterial_;

    // 前回並べたときの値（変わったときだけ並べ直す）
    float lastRadius_ = -1.0f;

    // 描画
    GameEngine::ModelComponent floor_;
    std::array<std::unique_ptr<GameEngine::ModelComponent>, kMaxRocks> rocks_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

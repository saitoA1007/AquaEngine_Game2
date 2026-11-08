#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include "Camera.h"
#include "Model.h"
#include "Material.h"
#include "WorldTransforms.h"
#include "FractureInstance.h"
#include "RenderQueue.h"
#include "DebugParameter.h"
#include "Matrix4x4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Application/Game/UI/HpShardPile.h"

/// <summary>
/// HPアイコンに使うモデル。GameSceneが取得して渡す。どれも管理クラスが所有する。
/// </summary>
struct HpHudModels {
    GameEngine::Model* frame = nullptr;   // 六角形の枠（hpFrame.gltf）
    GameEngine::Model* core = nullptr;    // 正八面体の核（hpCore.gltf）
    GameEngine::Model* shard = nullptr;   // 核を8個に分けた破片モデル（hpCoreShard.gltf。_cellの破壊グループが1つ）
};

/// <summary>
/// 3DのHPアイコン。六角形の枠の中で正八面体の核が回り、被弾すると核が砕けて、欠片が枠の中に積もる。
/// 画面に固定するため、毎フレーム画面の位置をカメラの逆変換でワールドへ戻し、カメラのすぐ前に置く。
/// 枠と核は、光を使わないインスタンス描画（SubmitInstancing）で、色をインスタンスごとに掛ける。
/// 欠片は、破片モデルをアイコンごとのFractureInstanceで描く（SubmitFracture）。破片の描画はモデル自身のマテリアルを使うため、
/// 欠片の色は全アイコンで共通になり、直近に砕けた核に合わせて変わる。
/// 演出は止まらない時間で進める。
/// </summary>
class HpHudView {
public:
    /// <summary>
    /// 材質・インスタンス・破片の領域を確保し、調整値を登録する。モデルが足りない、または破片モデルの形が合わなければ例外を投げる。
    /// </summary>
    /// <param name="camera">画面の位置をワールドへ戻すカメラ。シーンが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="models">枠・核・破片のモデル</param>
    /// <param name="whiteTextureHandle">単色の描画に使う白いテクスチャのハンドル</param>
    HpHudView(const GameEngine::Camera* camera, const HpHudModels& models, uint32_t whiteTextureHandle);
    ~HpHudView() = default;

    /// <summary>
    /// 表示するHPを設定する。PlayDamageの直後なら、減った分の核を砕く。毎フレーム呼ぶ。
    /// </summary>
    /// <param name="hp">現在のHP</param>
    /// <param name="maxHp">最大HP（kMaxIconsまで）</param>
    void SetHp(int32_t hp, int32_t maxHp);

    /// <summary>
    /// 被弾したことを伝える。次のSetHpで、減った分の核が砕ける。
    /// </summary>
    void PlayDamage() { isDamagePending_ = true; }

    /// <summary>
    /// 核の回転、色の変化、欠片の動きを進める。
    /// </summary>
    /// <param name="deltaTime">止まらない時間の経過（秒）</param>
    void Update(float deltaTime);

    /// <summary>
    /// 枠・核・欠片の位置と色を決めて、描画キューへ登録する。
    /// </summary>
    /// <param name="renderQueue">登録先。呼び出し元が所有する。</param>
    void Draw(GameEngine::RenderQueue* renderQueue);

private:
    // アイコン1つの状態
    struct Icon {
        bool isAlive = true;        // 核が残っているか
        float lostTime = 0.0f;      // 核を失ってからの時間（秒）。色の変化に使う
        HpShardPile pile;           // 砕けた欠片（失った後も積もって残る）
    };

    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 破片モデルから欠片の番号・中心・大きさを読み、アイコンごとのFractureInstanceを確保する。
    /// </summary>
    void SetUpShards();

    /// <summary>
    /// アイコンの数を最大HPに合わせる（kMaxIconsまで）。
    /// </summary>
    void ResizeIcons(int32_t count);

    /// <summary>
    /// 画面の位置を、カメラから前へdistanceだけ離れたワールドの位置へ戻す。
    /// </summary>
    /// <param name="screen">画面座標（1280x720）</param>
    Vector3 ScreenToWorld(const Vector2& screen) const;

    /// <summary>
    /// 失ってからの時間で、残りの色から失った色へ補間する。
    /// </summary>
    Vector4 LerpLostColor(const Vector4& aliveColor, const Vector4& lostColor, float lostTime) const;

private:
    // 調整値（置き方）
    Vector2 origin_ = { 40.0f, 40.0f };                     // 先頭のアイコンの左上の位置（画面座標）
    float iconSize_ = 64.0f;                                // アイコン1つの高さ（px。枠の上の頂点から下の頂点まで）
    float spacing_ = 4.0f;                                  // アイコンの間隔（px）
    float distance_ = 2.0f;                                 // カメラから前へ離す距離（ワールド単位。ニアクリップより遠く、ゲーム世界より近く）
    float spinSpeed_ = 1.5f;                                // 核の回る速さ（ラジアン/秒）

    // 調整値（色）
    Vector4 coreColor_ = { 0.4f, 0.8f, 1.0f, 1.0f };        // 残っている核の色
    Vector4 frameColor_ = { 0.3f, 0.6f, 0.75f, 1.0f };      // 核が残っているアイコンの枠の色
    Vector4 lostCoreColor_ = { 0.1f, 0.12f, 0.16f, 1.0f };  // 失った後の欠片の色
    Vector4 lostFrameColor_ = { 0.1f, 0.12f, 0.16f, 1.0f }; // 失った後の枠の色
    float shatterTime_ = 0.6f;                              // 砕けてから失った色になるまでの時間（秒）

    // 調整値（欠片の動き）
    HpShardPile::Settings pileSettings_;

    // 定数
    static constexpr int32_t kMaxIcons = 8;                 // アイコンの数の上限（インスタンスの確保数）
    static constexpr float kFrameHeight = 2.3f;             // 枠のモデルの高さ（モデル単位。外側の頂点の半径1.15の2倍）
    static constexpr float kFrameInnerRadius = 0.88f;       // 枠の内側の頂点の半径（モデル単位）
    static constexpr float kScreenWidth = 1280.0f;
    static constexpr float kScreenHeight = 720.0f;
    inline static const char* kDebugName = "HpHudView";

    // 参照（所有しない）
    const GameEngine::Camera* camera_ = nullptr;
    HpHudModels models_;
    uint32_t whiteTextureHandle_ = 0;
    GameEngine::Material* shardMaterial_ = nullptr;         // 破片モデル自身のマテリアル（破片の描画はこれを使う。モデルが所有）

    // 状態
    std::vector<Icon> icons_;
    int32_t shownHp_ = -1;            // 前回のSetHpで受け取ったHP（-1は未設定）
    bool isDamagePending_ = false;    // 被弾の通知を受け、次のSetHpで砕くか
    float spinAngle_ = 0.0f;          // 核の回転（ラジアン）。すべてのアイコンで同じ

    // 欠片の形（破片モデルの読み込み結果から作る）
    std::vector<HpShardPile::ShardShape> shardShapes_;

    // 描画
    GameEngine::Material material_;                                     // 枠と核の白い材質（色はインスタンスごと。インスタンス描画は光を使わない）
    GameEngine::WorldTransforms frameTransforms_;                       // 枠のインスタンス
    GameEngine::WorldTransforms coreTransforms_;                        // 核のインスタンス
    std::array<GameEngine::FractureInstance, kMaxIcons> shardInstances_; // アイコンごとの欠片

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

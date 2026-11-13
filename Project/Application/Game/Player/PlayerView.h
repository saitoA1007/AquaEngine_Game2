#pragma once
#include <memory>
#include "Model.h"
#include "ModelComponent.h"
#include "RenderQueue.h"
#include "DebugParameter.h"
#include "Vector3.h"
#include "Vector4.h"

/// <summary>
/// 自機の描画に使うモデル。GameSceneが取得して渡す。モデルは管理クラスが所有する。
/// </summary>
struct PlayerViewAssets {
    GameEngine::Model* core = nullptr;  // 宙に浮く正八面体の核（hpCore.gltf）
    GameEngine::Model* frame = nullptr; // 核を囲む六角形の枠（hpFrame.gltf。輪はXY平面にある）
};

/// <summary>
/// 自機の見た目。浮いた核と、それを囲んで交差して回る2つの六角形の枠（ジャイロスコープ）で描く。
/// 位置と状態（チャージの進み具合・連鎖中か・氷霧の発生）はPlayerから渡され、ゲームのルールは知らない。
/// 枠の回転はチャージで遅くなり、氷塊が飛んでいる間は速くなる。氷霧を出した瞬間は核と枠が一瞬大きくなる。
/// Update / Draw / PlayPulseの形を保てば、このクラスごと差し替えられる。
/// </summary>
class PlayerView {
public:
    /// <summary>
    /// 核と枠のモデルから描画の部品を作り、調整値を登録する。モデルが足りなければ例外を投げる。
    /// </summary>
    explicit PlayerView(const PlayerViewAssets& assets);

    /// <summary>
    /// 調整値を反映し、回転の速さ・回転角・拡縮の時間を進める。
    /// </summary>
    /// <param name="deltaTime">ゲームの経過時間（秒）</param>
    /// <param name="chargeRatio">チャージの進み具合（0〜1。チャージしていなければ0）</param>
    /// <param name="isChainActive">氷塊が1つでも飛んでいるか（連鎖中か）</param>
    void Update(float deltaTime, float chargeRatio, bool isChainActive);

    /// <summary>
    /// 氷霧を出した瞬間に呼ぶ。核と枠をまとめて一瞬大きくして戻す。
    /// </summary>
    void PlayPulse();

    /// <summary>
    /// 核と枠2つを描画キューへ登録する。浮かせる高さはここで足し、位置そのものは変えない。
    /// </summary>
    /// <param name="renderQueue">登録先</param>
    /// <param name="position">自機の位置（当たり判定の位置。この真上に浮かせて描く）</param>
    void Draw(GameEngine::RenderQueue* renderQueue, const Vector3& position);

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// モデルがnullptrなら例外を投げ、そうでなければそのまま返す。コンポーネントの初期化の前に確かめるために使う。
    /// </summary>
    static GameEngine::Model* RequireModel(GameEngine::Model* model);

    /// <summary>
    /// 回転の速さの倍率を、チャージと連鎖の目標へ滑らかに近づける。
    /// </summary>
    void UpdateSpinSpeed(float deltaTime, float chargeRatio, bool isChainActive);

    /// <summary>
    /// 氷霧の瞬間の拡縮を進め、今の倍率を求める。
    /// </summary>
    void UpdatePulse(float deltaTime);

    /// <summary>
    /// 描画の行列を作り、核と枠へ書き込む。
    /// </summary>
    /// <param name="position">自機の位置</param>
    void ApplyTransforms(const Vector3& position);

    /// <summary>
    /// 核と枠の色を、調整値から材質へ書き込む。
    /// </summary>
    void ApplyColors();

private:
    // 調整値（大きさ・位置）
    float viewScale_ = 0.6f;           // 見た目の基準の大きさ（モデルの大きさへ掛ける倍率）（仮）
    float liftHeight_ = 0.6f;          // 核の中心を、自機の位置から浮かせる高さ（仮）
    float frameScaleA_ = 1.0f;         // 枠Aの大きさ（核との重なりを防ぐため、2つで変える）（仮）
    float frameScaleB_ = 1.2f;         // 枠Bの大きさ（仮）

    // 調整値（回転）
    float spinSpeedA_ = 1.5f;          // 枠AがX軸まわりに回る基準の速さ（ラジアン/秒）（仮）
    float spinSpeedB_ = 1.1f;          // 枠BがZ軸まわりに回る基準の速さ（ラジアン/秒）（仮）
    float minChargeSpinScale_ = 0.15f; // フルチャージでの回転の速さの倍率（チャージしていなければ1）（仮）
    float chainSpinScale_ = 2.5f;      // 氷塊が飛んでいる間の回転の速さの倍率。チャージと重なったら掛け合わせる（仮）
    float spinFollowSpeed_ = 6.0f;     // 回転の速さが目標へ追いつく速さ（1/秒。大きいほど速く追いつく）（仮）

    // 調整値（氷霧の拡縮）
    float pulseScale_ = 1.3f;          // 氷霧の瞬間に一番大きくなったときの倍率（仮）
    float pulseTime_ = 0.25f;          // 大きくなって戻るまでの時間（秒）（仮）

    // 調整値（色。核と枠は別々。点滅は3つまとめて行うのでここでは扱わない）
    Vector4 coreColor_ = { 0.4f, 0.8f, 1.0f, 1.0f };  // 核の色（水色。仮）
    Vector4 frameColor_ = { 0.4f, 0.8f, 1.0f, 1.0f }; // 枠の色（水色。仮）

    // 定数
    static constexpr float kFlatAngle = 1.5707963f;   // 枠を水平（XZ平面）に寝かせる回転（ラジアン。X軸まわりに90度）
    static constexpr float kTwoPi = 6.2831853f;       // 回転角を巻き戻す周期（ラジアン）
    static constexpr float kPi = 3.1415927f;          // 拡縮の山の形（sin）に使う
    inline static const char* kDebugName = "PlayerView";

    // 状態
    float angleA_ = 0.0f;              // 枠Aの回転角（ラジアン）
    float angleB_ = 0.0f;              // 枠Bの回転角（ラジアン）
    float spinScale_ = 1.0f;           // 今の回転の速さの倍率（目標へ滑らかに近づく）
    float pulseTimer_ = 0.0f;          // 拡縮の残り時間（秒）。0なら出ていない
    float pulseMultiplier_ = 1.0f;     // 今の拡縮の倍率（核と枠に共通）

    // 描画（核と枠2つ。モデルのコンポーネントは別々）
    GameEngine::ModelComponent core_;
    GameEngine::ModelComponent frameA_;
    GameEngine::ModelComponent frameB_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

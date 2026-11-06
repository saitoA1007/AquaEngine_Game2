#pragma once
#include <memory>
#include "IGameObject.h"
#include "ModelComponent.h"
#include "InputCommand.h"
#include "DebugParameter.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Application/Game/Player/PlayerStats.h"
#include "Application/Game/Field/Field.h"
#include "Application/Game/Player/IceAreaView.h"

/// <summary>
/// プレイヤー。B1では移動とフィールド内への制限だけを行う。
/// </summary>
class Player : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 見た目・入力・フィールド・強化値を参照する。
    /// </summary>
    /// <param name="model">自機の見た目に使うモデル。管理クラスが所有する。</param>
    /// <param name="discModel">氷エリアの仮描画に使う球モデル。管理クラスが所有する。</param>
    /// <param name="inputCommand">入力機能。シーンが所有する。</param>
    /// <param name="field">移動制限に使うフィールド。GameObjectManagerが所有し、シーン終了まで有効。</param>
    /// <param name="stats">移動速度などの強化値。GameSceneが所有する。</param>
    Player(GameEngine::Model* model, GameEngine::Model* discModel, GameEngine::InputCommand* inputCommand, const Field* field, const PlayerStats* stats);
    ~Player() override = default;

    void Initialize() override;

    /// <summary>
    /// 入力から移動方向を作り、位置を更新してフィールド内へ収める。
    /// </summary>
    void Update() override;

    void Draw() override;

    /// <summary>
    /// 現在位置を取得する。
    /// </summary>
    const Vector3& GetPosition() const { return position_; }

    /// <summary>
    /// 自機の半径を取得する。
    /// </summary>
    float GetRadius() const { return radius_; }

    /// <summary>
    /// 氷エリアの半径を取得する。氷エリアはプレイヤーを中心に常に存在する。
    /// </summary>
    float GetIceAreaRadius() const { return iceAreaRadius_; }

    /// <summary>
    /// 現在のHPを取得する。
    /// </summary>
    int32_t GetHp() const { return hp_; }

    /// <summary>
    /// 無敵時間中かを返す。
    /// </summary>
    bool IsInvincible() const { return invincibleTimer_ > 0.0f; }

    /// <summary>
    /// ダメージを受ける。無敵時間中は何もしない。受けたら無敵時間を始める。
    /// </summary>
    /// <param name="damage">減らすHP</param>
    void TakeDamage(int32_t damage);

    /// <summary>
    /// 外から押し戻される。フィールド内に収め、見た目の位置も更新する。
    /// </summary>
    /// <param name="offset">押し戻す量（XZを使う）</param>
    void ApplyPushback(const Vector3& offset);

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 移動入力を長さ1以内のXZ方向として返す。画面上がワールド+Z。
    /// </summary>
    Vector3 GetMoveDirection() const;

private:
    // 調整値
    float radius_ = 0.5f; // 自機の半径（見た目と境界の余白）
    Vector4 color_ = { 0.4f, 0.8f, 1.0f, 1.0f }; // 水色系統（仮）
    float iceAreaRadius_ = 8.0f;   // 氷エリアの半径（仮）
    Vector3 iceAreaColor_ = { 0.08f, 0.25f, 0.40f }; // 氷エリアの仮描画の色（不透明）
    int32_t initialHp_ = 3;        // 初期HP
    float invincibleTime_ = 1.0f;  // 被弾後の無敵時間（秒）（仮）

    // 定数
    static constexpr float kStickThreshold = 0.2f; // スティックを入力として扱う最小の倒し量（入力コマンドの閾値に合わせる）

    // 状態
    Vector3 position_ = { 0.0f, 0.5f, 0.0f }; // XZは平面上の位置、Yは球が地面に接する高さ
    int32_t hp_ = 0;                          // 現在のHP
    float invincibleTimer_ = 0.0f;            // 無敵時間の残り（秒）

    // 参照（所有しない）
    GameEngine::InputCommand* inputCommand_ = nullptr;
    const Field* field_ = nullptr;
    const PlayerStats* stats_ = nullptr;

    // 描画
    GameEngine::ModelComponent modelComponent_;
    IceAreaView iceAreaView_; // 氷エリアの仮描画。差し替え可能

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

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

/// <summary>
/// プレイヤー。B1では移動とフィールド内への制限だけを行う。
/// </summary>
class Player : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 見た目・入力・フィールド・強化値を参照する。
    /// </summary>
    /// <param name="model">自機の見た目に使うモデル。管理クラスが所有する。</param>
    /// <param name="inputCommand">入力機能。シーンが所有する。</param>
    /// <param name="field">移動制限に使うフィールド。GameObjectManagerが所有し、シーン終了まで有効。</param>
    /// <param name="stats">移動速度などの強化値。GameSceneが所有する。</param>
    Player(GameEngine::Model* model, GameEngine::InputCommand* inputCommand, const Field* field, const PlayerStats* stats);
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

    // 定数
    static constexpr float kStickThreshold = 0.2f; // スティックを入力として扱う最小の倒し量（入力コマンドの閾値に合わせる）

    // 状態
    Vector3 position_ = { 0.0f, 0.5f, 0.0f }; // XZは平面上の位置、Yは球が地面に接する高さ

    // 参照（所有しない）
    GameEngine::InputCommand* inputCommand_ = nullptr;
    const Field* field_ = nullptr;
    const PlayerStats* stats_ = nullptr;

    // 描画
    GameEngine::ModelComponent modelComponent_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

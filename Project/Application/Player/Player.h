#pragma once
#include "IGameObject.h"
#include "WorldTransform.h"
#include "InputCommand.h"
#include "DebugParameter.h"
#include "Collider.h"

#include "PlayerAction.h"
#include "Application/Camera/CameraController.h"
#include "PlayerEffectManager.h"

// 前方宣言
class PlayerEffectManager;

class Player : public GameEngine::IGameObject {
public:
	Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model, GameEngine::AnimationManager* animationManager, PlayerEffectManager* effectManager);
	~Player() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	/// <summary>
	/// ワールド行列を取得
	/// </summary>
	/// <returns></returns>
	GameEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

	/// <summary>
	/// プレイヤーの位置を取得
	/// </summary>
	/// <returns></returns>
	Vector3 GetPlayerPos() { return worldTransform_.GetWorldPosition(); }

	// カメラのワールド行列を取得
	void SetCamera(CameraController* camera) {
		camera_ = camera;
	}

	// 現在のHpを取得
	int32_t GetCurrentHp() const { return playerStatus_.GetCurrentHp(); }

	// 最大のHpを取得
	int32_t GetMaxHp() const { return playerStatus_.GetMaxHp(); }

	// 現在の状態を取得
	PlayerState GetCurrentState() const { return commonData_.state; }

	// 攻撃力
	float GetDamage() const { return playerAttackDownAction_.GetAttackDownPower(); }

	// 突進の最大速度
	float GetRushMaxSpeed() const { return attackRushAction_.GetRushMaxSpeed(); }

	// 速度を取得
	Vector3 GetVelocity() const { return commonData_.velocity; }

	bool IsHitWall() const { return bounceAction_.IsHitWall(); }
	void SetIsHitWall(bool isHitWall) { bounceAction_.SetIsHitWall(isHitWall); }

	// 描画の有効を設定
	void SetIsDraw(bool isDraw) {
		isDraw_ = isDraw;
	}

	// ヒットエフェクトを開始
	void StartHitEffect(Vector3 pos) {
		effectManager_->StartHitEffect(pos, playerAttackDownAction_.GetPowerLevel());
	}

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;
	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;
	// モデル
	GameEngine::Model* model_ = nullptr;
	// カメラ機能
	CameraController* camera_ = nullptr;

	// ワールド行列
	GameEngine::WorldTransform worldTransform_;

	// 球の当たり判定
	GameEngine::SphereCollider collider_;

	// アニメーション管理
	std::unique_ptr<PlayerAnimator> animator_;

	// 仮のプレイヤーのエフェクト管理システム
	PlayerEffectManager* effectManager_ = nullptr;

	// 表示フラグ
	bool isDraw_ = true;

private: 
	// プレイヤー
	PlayerCommonData commonData_;

	// 移動アクション
	PlayerMoveAction moveAction_;
	// 突進アクション
	PlayerAttackRushAction attackRushAction_;
	// 跳ね返りアクション
	PlayerBounceAction bounceAction_;
	// 落下攻撃アクション
	PlayerAttackDownAction playerAttackDownAction_;

	// プレイヤーが受ける物理
	PlayerPhysics playerPhysics_;

	// プレイヤーの状態
	PlayerStatus playerStatus_;

private:

	// 制限
	void ApplyClamp();

	// 回転の更新
	void UpdateRotation();

	// 当たり判定
	void OnCollisionStay(const GameEngine::CollisionResult& result);
};

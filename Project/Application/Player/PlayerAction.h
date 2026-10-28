#pragma once
#include "IPlayerAction.h"

// プレイヤーの状態
class PlayerStatus : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData);

	void Update();

	// パラメータを登録する
	void RegisterParameter(GameEngine::DebugParameter* param) override;

public:

	// ダメージを受ける
	void TakeDamage(int32_t damage) {
		if (isCoolActive_) { return; }
		currentHp_ -= damage;
		isCoolActive_ = true;
	}

	// 現在Hp
	int32_t GetCurrentHp()const { return currentHp_; }

	// 最大Hp
	int32_t GetMaxHp()const { return maxHp_; }

	// 体力をリセット
	void ResetHp() { currentHp_ = maxHp_; }

private:

	int32_t maxHp_ = 3;

	float coolTime_ = 2.0f;

private:
	// クール状態
	bool isCoolActive_ = false;

	int32_t currentHp_ = 1;

	float timer_ = 0.0f;
};

// プレイヤーが常に受ける物理
class PlayerPhysics : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData);

	void Update();

	// パラメータを登録する
	void RegisterParameter(GameEngine::DebugParameter* param) override;

	// 落下の最大速度を取得
	float GetMaxFallSpeed() const { return kMaxFallSpeed_; }

private:
	// 落下速度の上限
	float kMaxFallSpeed_ = 2.0f;
	// 落下加減速量
	float kFallAcceleration_ = -9.6f;
};

// 移動アクション
class PlayerMoveAction : public IPlayerAction {
public:
	// 初期化
	void Initialize(PlayerCommonData* commonData, GameEngine::InputCommand* inputCommand);

	// 入力
	void ProcessInput();

	// カメラ基準のベクトルを更新する
	void UpdateCameraBasis(const Matrix4x4& cameraWorldMatrix);

	// パラメータを登録する
	void RegisterParameter(GameEngine::DebugParameter* param) override;

private:
	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;

private:
	// 地上の移動速度
	float kGroundMaxMoveSpeed_ = 16.0f;
	// 空中の移動速度
	float kAirMaxMoveSpeed_ = 16.0f;
	// 地上での加速量
	float kGroundAcceleration_ = 8.0f;
	// 空中での加速量
	float kAirAcceleration_ = 2.0f;
	// 地上での減速量
	float kGroundDeceleration_ = 2.0f;
	// 空中での減速量
	float kAirDeceleration_ = 1.0f;
private:

	// カメラ基準
	Vector3 cameraForwardXZ_ = { 0.0f,0.0f,1.0f };
	Vector3 cameraRightXZ_ = { 1.0f,0.0f,0.0f };

private:

	float MoveTowards(float current, float target, float maxDelta);
};

// 突進アクション
class PlayerAttackRushAction : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData, GameEngine::InputCommand* inputCommand);

	void ProcessInput();

	void Update();

	// パラメータを登録する
	void RegisterParameter(GameEngine::DebugParameter* param) override;

	// 突進の最大速度を取得
	float GetRushMaxSpeed() const { return  kRushMaxSpeed_; }

private:
	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;

private:
	// 突撃予備動作最大時間
	float kPreRushMaxTime_ = 0.5f;
	// 突撃最大速度
	float kRushMaxSpeed_ = 32.0f;
	// 突進時硬直最大時間
	float kRushLockMaxTime_ = 1.0f;
	// 突進溜め最大時間
	float kRushChargeMaxTime_ = 2.0f;
	// 突進の強さLv1になるまでの時間の割合
	float kRushChargeLevel1Ratio_ = 0.0f;
	// 突進の強さLv2になるまでの時間の割合
	float kRushChargeLevel2Ratio_ = 0.5f;
	// 突進の強さLv3になるまでの時間の割合
	float kRushChargeLevel3Ratio_ = 1.0f;
	// 突進時クールタイム（秒。硬直終了後に次の突進が可能になるまでの時間）
	float kRushCooldownTime_ = 0.5f;

	// 突撃の強さ
	float kRushStrengthLevel1_ = 0.5f;
	float kRushStrengthLevel2_ = 0.8f;
	float kRushStrengthLevel3_ = 1.0f;

	// 突進している時の時間
	float kRushMaxTime_ = 2.0f;

private:
	float chargeTimer_ = 0.0f;
	Vector3 rushDirection_;
	uint32_t rushChargeLevel_ = 0;

	float rushTimer_ = 0.0f;
	float coolTime_ = 0.0f;
};

// 跳ね返りアクション
class PlayerBounceAction : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData);

	void WallBounce(Vector3& pos, const Vector3& bounceDirection, const float& penetrationDepth, const float kRushMaxSpeed);

	// パラメータを登録する
	void RegisterParameter(GameEngine::DebugParameter* param) override;

	bool IsHitWall() const { return isHitWall_; }
	void SetIsHitWall(bool isHitWall) { isHitWall_ = isHitWall; }

private:
	// 跳ね上がり後の高さ
	float kWallBounceUpSpeed_ = 10.0f;
	// 跳ね上がり後の壁から離れる距離
	float kWallBounceAwaySpeed_ = 5.0f;
	// 跳ね返り直後の硬直時間
	float kWallBounceLockTime_ = 0.8f;

	// 速さに応じた跳ね返りの倍率の最低値
	float kWallBounceMinSpeedFactor_ = 0.5f;
	// 速さに応じた跳ね返りの倍率の最大値
	float kWallBounceMaxSpeedFactor_ = 1.5f;

	// 突進の時の壁に衝突した際の跳ね返り倍率
	float kWallBounceReflectFactor_ = 1.0f;
	// 通常の時の壁に衝突した際の跳ね返り倍率
	float kWallHitReflectFactor_ = 0.2f;

	// 壁のヒットフラグ
	bool isHitWall_ = false;
};

// 急降下攻撃アクション
class PlayerAttackDownAction : public IPlayerAction {
public:
	void Initialize(PlayerCommonData* commonData, GameEngine::InputCommand* inputCommand);

	void ProcessInput();

	void Update();

	// パラメータを登録する
	void RegisterParameter(GameEngine::DebugParameter* param) override;

	// 落下の最大速度
	float GetAttackDownMaxSpeed()const { return kAttackDownMaxSpeed_; }

	float GetAttackDownPower() const { return attackDownPower_; }

	// 攻撃のレベル
	uint32_t GetPowerLevel() const {
		float diff = attackDownPower_ - kAttackDownMinPower_;
		float ratio = diff / kAttackDownMaxPower_;

		uint32_t level = 1;
		if (ratio > 0.4f && ratio <= 0.7f) {
			level = 2;
		} else if(ratio > 0.7f) {
			level = 3;
		}
		return level;
	}

private:
	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;

private:
	// 落下前の硬直時間
	float kAttackPreDownTime_ = 0.3f;
	// 落下攻撃の最大落下速度
	float kAttackDownMaxSpeed_ = 30.0f;
	// 落下攻撃の最低攻撃力
	float kAttackDownMinPower_ = 1.0f;
	// 落下攻撃の最大攻撃力
	float kAttackDownMaxPower_ = 10.0f;
	// 最大攻撃力に達するまでの落下距離
	float kAttackDownDistanceToMax_ = 5.0f;
	// 急降下準備中の上昇量
	float kAttackDownPrepareRise_ = 20.0f;

	// 落下攻撃の加速度
	float kAttackDownAcceleration_ = -70.0f;

private:
	// 攻撃の強さ
	float attackDownPower_ = 0.0f;

	bool isAttackDownPrepping_ = true;
	float attackDownPrepareTimer_ = 0.0f;
};

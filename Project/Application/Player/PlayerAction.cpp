#include "pch.h"
#define NOMINMAX
#include "PlayerAction.h"
#include <algorithm>
#include "InputCommand.h"
#include "MyMath.h"
#include "FPSCounter.h"
#include "DebugParameter.h"
#include "EasingManager.h"
#include "LogManager.h"
#include "PlayerEffectManager.h"
using namespace GameEngine;

//=====================================================
// プレイヤーが受ける物理
//=====================================================

void PlayerStatus::Initialize(PlayerCommonData* commonData) {
	commonData_ = commonData;

	currentHp_ = maxHp_;
}

void PlayerStatus::Update() {
	if (!isCoolActive_) { return; }

	timer_ += FpsCounter::gameDeltaTime / coolTime_;

	if (timer_ >= 1.0f) {
		timer_ = 0.0f;
		isCoolActive_ = false;
	}
}

void PlayerStatus::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "Status";
	int index = 0;

	param->Register("MaxHp", maxHp_, index++, subGroup);
	param->Register("CoolTime", coolTime_, index++, subGroup);
}

//=====================================================
// プレイヤーが受ける物理
//=====================================================

void PlayerPhysics::Initialize(PlayerCommonData* commonData) {
	commonData_ = commonData;
}

void PlayerPhysics::Update() {
	// 重力を適応
	commonData_->velocity.y += kFallAcceleration_ * FpsCounter::gameDeltaTime;

	if (commonData_->state == PlayerState::kJump) {
		if (commonData_->velocity.y <= 1.0f) {
			if (commonData_->animator_->GetCurrentType() != PlayerAnimationType::kAirMove) {
				// 空中アニメーション
				commonData_->animator_->StartAnimation(PlayerAnimationType::kAirMove, "AirMove", 0.1f);
			}
		}
	}
}

void PlayerPhysics::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "Physics";
	int index = 0;

	param->Register("FallAcceleration", kFallAcceleration_, index++, subGroup);
}

//======================================================
// プレイヤーの移動アクション
//======================================================

void PlayerMoveAction::Initialize(PlayerCommonData* commonData, GameEngine::InputCommand* inputCommand) {
	commonData_ = commonData;
	inputCommand_ = inputCommand;
}

void PlayerMoveAction::ProcessInput() {

	bool isJump = commonData_->state == PlayerState::kJump;

	Vector3 dir = { 0,0,0 };
	// XZの目標速度
	Vector3 desiredVelocityXZ = { 0,0,0 };

	// 移動の操作
	if (inputCommand_->IsCommandActive("MoveUp")) { dir -= cameraForwardXZ_; }
	if (inputCommand_->IsCommandActive("MoveDown")) { dir += cameraForwardXZ_; }
	if (inputCommand_->IsCommandActive("MoveLeft")) { dir -= cameraRightXZ_; }
	if (inputCommand_->IsCommandActive("MoveRight")) { dir += cameraRightXZ_; }

	// 目標方向
	if (commonData_->state != PlayerState::kAttackRush) {
		commonData_->targetDir = dir;
	}

	if (commonData_->state == PlayerState::kNone || commonData_->state == PlayerState::kJump) {
		if (dir.x != 0.0f || dir.z != 0.0f) {
			dir.y = 0.0f;
			dir.Normalize();

			// 最大移動速度を受け取る
			const float maxSpeed = isJump ? kAirMaxMoveSpeed_ : kGroundMaxMoveSpeed_;
			// 目標速度を設定
			desiredVelocityXZ = dir * maxSpeed;
		}
	}

	// 加速,減速
	Vector3 target = { 0,0,0 };
	float deltaSpeed = 0.0f;
	if (desiredVelocityXZ.x == 0.0f && desiredVelocityXZ.z == 0.0f) {
		// 減速
		const float deceleration = isJump ? kAirDeceleration_ : kGroundDeceleration_;
		deltaSpeed = deceleration * FpsCounter::gameDeltaTime;
	} else {
		// 加速
		const float acceleration = isJump ? kAirAcceleration_ : kGroundAcceleration_;
		deltaSpeed = acceleration * FpsCounter::gameDeltaTime;
		target = desiredVelocityXZ;
	}

	// 適応
	commonData_->velocity.x = MoveTowards(commonData_->velocity.x, target.x, deltaSpeed);
	commonData_->velocity.z = MoveTowards(commonData_->velocity.z, target.z, deltaSpeed);
}

void PlayerMoveAction::UpdateCameraBasis(const Matrix4x4& cameraWorldMatrix) {
	// カメラからのZ軸
	Vector3 forward = {
	-cameraWorldMatrix.m[2][0],
	-cameraWorldMatrix.m[2][1],
	-cameraWorldMatrix.m[2][2]
	};
	forward.y = 0.0f;
	if (forward.x != 0.0f || forward.z != 0.0f) {
		forward.Normalize();
	}

	// カメラからのX軸
	Vector3 right = {
		cameraWorldMatrix.m[0][0],
		cameraWorldMatrix.m[0][1],
		cameraWorldMatrix.m[0][2]
	};
	right.y = 0.0f;
	if (right.x != 0.0f || right.z != 0.0f) {
		right.Normalize();
	}

	cameraForwardXZ_ = forward;
	cameraRightXZ_ = right;
}

float PlayerMoveAction::MoveTowards(float current, float target, float maxDelta) {
	const float diff = target - current;

	if (std::fabs(diff) <= maxDelta) {
		return target;
	}

	return current + (diff > 0.0f ? maxDelta : -maxDelta);
}

void PlayerMoveAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "Move";
	int index = 0;
	param->Register("GroundMaxMoveSpeed", kGroundMaxMoveSpeed_, index++, subGroup);
	param->Register("AirMaxMoveSpeed", kAirMaxMoveSpeed_, index++, subGroup);
	param->Register("GroundAcceleration", kGroundAcceleration_, index++, subGroup);
	param->Register("AirAcceleration", kAirAcceleration_, index++, subGroup);
	param->Register("GroundDeceleration", kGroundDeceleration_, index++, subGroup);
	param->Register("AirDeceleration", kAirDeceleration_, index++, subGroup);
}

//=======================================================
// プレイヤーの突進アクション
//=======================================================

void PlayerAttackRushAction::Initialize(PlayerCommonData* commonData, GameEngine::InputCommand* inputCommand) {
	commonData_ = commonData;
	inputCommand_ = inputCommand;
}

void PlayerAttackRushAction::ProcessInput() {
	// ため状態
	if (commonData_->state == PlayerState::kNone) {
		if (inputCommand_->IsCommandActive("RushCharge")) {
			chargeTimer_ = 0.0f;
			// Rush方向初期化
			Vector3 rushDirXZ = commonData_->currentDir;
			if (rushDirXZ.x == 0.0f && rushDirXZ.z == 0.0f) { rushDirXZ = { commonData_->velocity.x, 0.0f, commonData_->velocity.z }; }
			rushDirection_ = (rushDirXZ.x == 0.0f && rushDirXZ.z == 0.0f) ? Vector3(0, 0, 1) : rushDirXZ.Normalize();
			// 溜め開始時はレベルをリセット
			rushChargeLevel_ = 0;
			// 現在の状態
			commonData_->state = PlayerState::kCharging;
			// 溜めアニメーション
			commonData_->animator_->StartAnimation(PlayerAnimationType::kRushAttack, "突進_Prepare", kRushChargeMaxTime_, false);
			Log("Player start charge");
		}
	}

	// 発射状態
	if (commonData_->state == PlayerState::kCharging) {
		if (inputCommand_->IsCommandActive("RushStart")) {
			// 予備動作時間を溜め比率で決定
			float chargeRatio_ = std::clamp(chargeTimer_, 0.0f, 1.0f);
			// 溜め比率に応じてレベル決定
			if (chargeRatio_ < kRushChargeLevel2Ratio_) {
				rushChargeLevel_ = 1;
			} else if (chargeRatio_ < kRushChargeLevel3Ratio_) {
				rushChargeLevel_ = 2;
			} else {
				rushChargeLevel_ = 3;
			}
			rushTimer_ = 0.0f;
			coolTime_ = 0.0f;

			float levelMultiplier = 1.0f;
			switch (rushChargeLevel_) {
			case 1: levelMultiplier = kRushStrengthLevel1_; break;
			case 2: levelMultiplier = kRushStrengthLevel2_; break;
			case 3: levelMultiplier = kRushStrengthLevel3_; break;
			default: levelMultiplier = kRushStrengthLevel1_; break;
			}
			float rushSpeed = kRushMaxSpeed_ * levelMultiplier;
			Vector3 initVel = rushDirection_ * rushSpeed;
			commonData_->velocity.x = initVel.x;
			commonData_->velocity.z = initVel.z;
			// 現在の状態
			commonData_->state = PlayerState::kAttackRush;
			// 突進アニメーション
			commonData_->animator_->StartAnimation(PlayerAnimationType::kRushAttack, "突進_Main", 0.2f, false);
			// チャージエフェクトを無効
			commonData_->effectManager_->StartChargeEffect({}, {}, false);
			Log("Player start attackRush");
		}
	}
}

void PlayerAttackRushAction::Update() {

	// ため時間計測
	if (commonData_->state == PlayerState::kCharging) {
		chargeTimer_ += FpsCounter::gameDeltaTime / kRushChargeMaxTime_;

		// 予備動作時間を溜め比率で決定
		float chargeRatio_ = std::clamp(chargeTimer_, 0.0f, 1.0f);
		// 溜め比率に応じてレベル決定
		if (chargeRatio_ < kRushChargeLevel2Ratio_) {
			rushChargeLevel_ = 1;
		} else if (chargeRatio_ < kRushChargeLevel3Ratio_) {
			rushChargeLevel_ = 2;
		} else {
			rushChargeLevel_ = 3;
		}

		Vector4 color = {};
		switch (rushChargeLevel_) {
		case 1: color = { 0.88f,1.0f,0.0f,1.0f}; break;
		case 2: color = { 1.0f,0.6f,0.0f,1.0f }; break;
		case 3: color = { 1.0f,0.09f,0.0f,1.0f }; break;
		default: color = { 0.88f,1.0f,0.0f,1.0f }; break;
		}

		// チャージ演出
		commonData_->effectManager_->StartChargeEffect(commonData_->transform.translate, color, true);
	}

	// 突進
	if (commonData_->state == PlayerState::kAttackRush) {
		rushTimer_ += FpsCounter::gameDeltaTime / kRushMaxTime_;

		if (rushTimer_ >= 1.0f) {
			Log("Player end attackRush");
			commonData_->state = PlayerState::kStiffness;
			commonData_->animator_->StartAnimation(PlayerAnimationType::kWalk, "歩き");
		}
	}

	// 突進硬直のクールタイム
	if (commonData_->state == PlayerState::kStiffness) {
		coolTime_ += FpsCounter::gameDeltaTime / kRushCooldownTime_;

		if (coolTime_ >= 1.0f) {
			commonData_->state = PlayerState::kNone;
		}
	}

}

void PlayerAttackRushAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "AttackRush";
	int index = 0;
	param->Register("PreRushMaxTime", kPreRushMaxTime_, index++, subGroup);
	param->Register("RushMaxSpeed", kRushMaxSpeed_, index++, subGroup);
	param->Register("RushLockMaxTime", kRushLockMaxTime_, index++, subGroup);
	param->Register("RushChargeMaxTime", kRushChargeMaxTime_, index++, subGroup);

	param->Register("RushChargeLevel1Ratio", kRushChargeLevel1Ratio_, index++, subGroup);
	param->Register("RushChargeLevel2Ratio", kRushChargeLevel2Ratio_, index++, subGroup);
	param->Register("RushChargeLevel3Ratio", kRushChargeLevel3Ratio_, index++, subGroup);

	param->Register("RushCooldownTime", kRushCooldownTime_, index++, subGroup);

	param->Register("RushStrengthLevel1", kRushStrengthLevel1_, index++, subGroup);
	param->Register("RushStrengthLevel2", kRushStrengthLevel2_, index++, subGroup);
	param->Register("RushStrengthLevel3", kRushStrengthLevel3_, index++, subGroup);

	param->Register("RushMaxTime", kRushMaxTime_, index++, subGroup);
}

//=======================================================
// プレイヤーの跳ね返りアクション
//=======================================================

void PlayerBounceAction::Initialize(PlayerCommonData* commonData) {
	commonData_ = commonData;
}

void PlayerBounceAction::WallBounce(Vector3& pos, const Vector3& bounceDirection, const float& penetrationDepth, const float kRushMaxSpeed) {

	// ラッシュ状態からの突進であれば上に飛ぶ
	if (commonData_->state == PlayerState::kAttackRush) {

		// ヒットフラグを有効
		isHitWall_ = true;

		// 跳ね返りアニメーション
		commonData_->animator_->StartAnimation(PlayerAnimationType::kRushAttack, "突進_End", 1.0f, false);

		//currentBounceLockTime_ = kWallBounceLockTime_;

		Vector3 prevHoriz = { commonData_->velocity.x, 0.0f, commonData_->velocity.z };
		float prevSpeed = prevHoriz.Length();
		// 速度比を計算
		float speedRatio = 0.0f;
		if (kRushMaxSpeed > 0.00001f) {
			speedRatio = std::clamp(prevSpeed / kRushMaxSpeed, 0.0f, 1.0f);
		}
		// 高さ倍率を決定
		float heightMultiplier = Lerp(kWallBounceMinSpeedFactor_, kWallBounceMaxSpeedFactor_, speedRatio);

		// 水平方向の反発速度は従来通り
		commonData_->velocity = bounceDirection * (kWallBounceAwaySpeed_ * kWallBounceReflectFactor_);
		// 上方向速度は強さと速度倍率に応じて変化
		commonData_->velocity.y = kWallBounceUpSpeed_ * kWallBounceReflectFactor_ * heightMultiplier;

		commonData_->state = PlayerState::kJump;
	} else {

		// 壁に衝突していたら押し戻す
		Vector3 dirXZ = { bounceDirection.x, 0.0f, bounceDirection.z };
		if (dirXZ.x != 0.0f || dirXZ.z != 0.0f) { dirXZ.Normalize(); }
		float depth = std::max(penetrationDepth, 0.0f);
		Vector3 correction = { dirXZ.x * depth, 0.0f, dirXZ.z * depth };
		pos.x += correction.x;
		pos.z += correction.z;

		// 速度と方向を変更する
		Vector3 velocityXZ = { commonData_->velocity.x, 0.0f, commonData_->velocity.z };
		float dot = velocityXZ.Dot(dirXZ);
		if (dot < 0.0f) {
			Vector3 reflected = {
				velocityXZ.x - 2.0f * dot * dirXZ.x,
				0.0f,
				velocityXZ.z - 2.0f * dot * dirXZ.z
			};
			commonData_->velocity.x = reflected.x * kWallHitReflectFactor_;
			commonData_->velocity.z = reflected.z * kWallHitReflectFactor_;
			Vector3 newDir = { reflected.x, 0.0f, reflected.z };
			float len = newDir.Length();
			if (len > 0.00001f) {
				commonData_->targetDir = Vector3::Normalize(newDir);
			}
		}
	}
}

void PlayerBounceAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "Bounce";
	int index = 0;
	param->Register("WallBounceUpSpeed", kWallBounceUpSpeed_, index++, subGroup);
	param->Register("WallBounceAwaySpeed", kWallBounceAwaySpeed_, index++, subGroup);
	param->Register("WallBounceLockTime", kWallBounceLockTime_, index++, subGroup);

	param->Register("WallBounceMinSpeedFactor", kWallBounceMinSpeedFactor_, index++, subGroup);
	param->Register("WallBounceMaxSpeedFactor", kWallBounceMaxSpeedFactor_, index++, subGroup);

	param->Register("WallBounceReflectFactor", kWallBounceReflectFactor_, index++, subGroup);
	param->Register("WallHitReflectFactor", kWallHitReflectFactor_, index++, subGroup);
}

//=======================================================
// プレイヤーの急降下攻撃アクション
//=======================================================

void PlayerAttackDownAction::Initialize(PlayerCommonData* commonData, GameEngine::InputCommand* inputCommand) {
	commonData_ = commonData;
	inputCommand_ = inputCommand;
}

void PlayerAttackDownAction::ProcessInput() {

	if (commonData_->state == PlayerState::kJump) {
		if (inputCommand_->IsCommandActive("AttackDown")) {
			commonData_->velocity.x = 0.0f;
			commonData_->velocity.z = 0.0f;
			attackDownPower_ = 0.0f;
			// 準備状態に入る
			isAttackDownPrepping_ = true;
			attackDownPrepareTimer_ = 0.0f;

			// 攻撃力計算
			float fallDistance = commonData_->transform.translate.y;
			if (fallDistance < 0.0f) fallDistance = 0.0f;
			float ratio = 0.0f;
			if (kAttackDownDistanceToMax_ > 0.00001f) {
				ratio = std::clamp(fallDistance / kAttackDownDistanceToMax_, 0.0f, 1.0f);
			}
			attackDownPower_ = Lerp(kAttackDownMinPower_, kAttackDownMaxPower_, ratio);

			commonData_->state = PlayerState::kAttackDown;

			// 落下攻撃アニメーション
			commonData_->animator_->StartAnimation(PlayerAnimationType::kDownAttack, "DownAttack_Prepare", 1.0f, false);

			Log("Player start attackDown");
		}
	}
}

void PlayerAttackDownAction::Update() {

	if (commonData_->state == PlayerState::kAttackDown) {

		if (isAttackDownPrepping_) {

			attackDownPrepareTimer_ += FpsCounter::gameDeltaTime / kAttackPreDownTime_;

			commonData_->velocity.y += kAttackDownPrepareRise_ * FpsCounter::gameDeltaTime;

			if (attackDownPrepareTimer_ >= 1.0f) {
				isAttackDownPrepping_ = false;
			}

		} else {
			// 下への加速度
			commonData_->velocity.y += kAttackDownAcceleration_ * FpsCounter::gameDeltaTime;

			// 落下エフェクトを再生
			commonData_->effectManager_->StartDownAttackEffect(commonData_->transform.translate, true);
		}
	} else {
		// 落下エフェクトの停止
		commonData_->effectManager_->StartDownAttackEffect({}, false);
	}
}

void PlayerAttackDownAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "AttackDown";
	int index = 0;

	param->Register("AttackPreDownTime", kAttackPreDownTime_, index++, subGroup);
	param->Register("AttackDownMaxSpeed", kAttackDownMaxSpeed_, index++, subGroup);
	param->Register("AttackDownMinPower", kAttackDownMinPower_, index++, subGroup);
	param->Register("AttackDownMaxPower", kAttackDownMaxPower_, index++, subGroup);
	param->Register("AttackDownDistanceToMax", kAttackDownDistanceToMax_, index++, subGroup);
	param->Register("AttackDownPrepareRise", kAttackDownPrepareRise_, index++, subGroup);
	param->Register("AttackDownAcceleration", kAttackDownAcceleration_, index++, subGroup);
}
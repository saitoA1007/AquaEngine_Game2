#include "pch.h"
#define NOMINMAX
#include "BossBattleAction.h"
#include <numbers>
#include "FPSCounter.h"
#include "MyMath.h"
#include "EasingManager.h"
#include "RandomGenerator.h"
#include "Application/Enemy/BossAnimator.h"
#include "DebugParameter.h"
#include "Application/Enemy/BossRangedAttackManager.h"
using namespace GameEngine;

//=============================================================
// 突進攻撃
//=============================================================

BossRushAttackAction::BossRushAttackAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void BossRushAttackAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;
	state_ = State::kMove;

	Vector3 myDir = commonData_.transform.translate;
	myDir.y = 0.0f;
	myDir.Normalize();
	// 最初の角度
	startAngle_ = std::atan2f(myDir.z, myDir.x);

	Vector3 playerDir = *commonData_.playerPos;
	playerDir.y = 0.0f;
	playerDir.Normalize();
	playerDir = playerDir * -1.0f;
	float tmpEndAngle = std::atan2f(playerDir.z, playerDir.x);
	float diffAngle = Math::GetAngleDiff(startAngle_, tmpEndAngle);
	// 最後の角度
	endAngle_ = startAngle_ + diffAngle;

	// 突進の位置を取得
	startRushPos_ = playerDir * commonData_.stageRadius;
	startRushPos_.y = defaultPosY_;
	endRushPos_ = myDir * commonData_.stageRadius;
	endRushPos_.y = defaultPosY_;

	commonData_.animator->StartAnimation(BossAnimationType::kRush, "Rush_End", rushMaxTime_, false);
}

void BossRushAttackAction::Update() {

	switch (state_)
	{
	case BossRushAttackAction::State::kMove:
		RotateMove();
		break;


	case BossRushAttackAction::State::kRush:
		RushAttack();
		break;
	}
}

void BossRushAttackAction::Finalize() {
	// 途中で中断された場合も突進エフェクトを無効化する
	commonData_.rangedAttackManager->SetRush(commonData_.transform.translate, { 0.0f,0.0f,1.0f }, false);
}

void BossRushAttackAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "RushAttack";
	int index = 0;

	param->Register("RotateMoveMaxTime", rotateMoveMaxTime_, index++, subGroup);
	param->Register("RushMaxTime", rushMaxTime_, index++, subGroup);
	param->Register("RushDistanceRatio", rushDistanceRatio_, index++, subGroup);
}

void BossRushAttackAction::RotateMove() {
	timer_ += FpsCounter::gameDeltaTime / rotateMoveMaxTime_;

	// 角度補間
	float preAngle = angle_;
	angle_ = Lerp(startAngle_, endAngle_, timer_);

	float r = 0.0f;
	if (timer_ >= 0.4f && timer_ < 0.8f) {
		float localT = (timer_ - 0.4f) / 0.4f;
		r = Lerp(0.0f, 2.0f, localT);
	} else if (timer_ >= 0.8f) {
		float localT = (timer_ - 0.8f) / 0.2f;
		r = Lerp(2.0f, 0.0f, localT);
	}

	// 膨らむような円の軌跡
	float radius = commonData_.stageRadius + r;

	// 回転移動
	commonData_.transform.translate = GetXZFromAngle(angle_, radius, defaultPosY_);
	
	// 回転
	commonData_.transform.rotate.z = 0.2f;
	Vector3 prePos = GetXZFromAngle(preAngle, radius, defaultPosY_);
	Vector3 dir = commonData_.transform.translate - prePos;
	dir.Normalize();
	commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

	// 突進エフェクト
	commonData_.rangedAttackManager->SetRush(commonData_.transform.translate + (dir * 4.0f), dir, true);

	if (timer_ >= 1.0f) {
		state_ = State::kRush;
		timer_ = 0.0f;

		// プレイヤーの位置から突進する位置を求める
		Vector3 targetDir = *commonData_.playerPos - commonData_.transform.translate;
		targetDir.y = 0.0f;
		targetDir.Normalize();

		// ボスのステージの中心方向へのベクトル
		Vector3 myDir = commonData_.transform.translate * -1.0f;
		myDir.y = 0.0f;
		myDir.Normalize();

		float myAngle = std::atan2f(myDir.z, myDir.x);
		float targetAngle = std::atan2f(targetDir.z, targetDir.x);
		// 現在の方向と突進する方向の差を求める
		float diffAngle = Math::GetAngleDiff(myAngle, targetAngle);

		// 角度差を範囲に制限する
		float limitAngle = std::numbers::pi_v<float> / 6.0f;
		if (diffAngle > limitAngle) {
			diffAngle = limitAngle;
		} else if(diffAngle < -limitAngle) {
			diffAngle = -limitAngle;
		}

		// 突進する方向を求める
		float rushAngle = myAngle + diffAngle;
		targetDir = Vector3(std::cosf(rushAngle), 0.0f, std::sinf(rushAngle));

		// 突進する位置を求める
		float rushDistance = commonData_.stageRadius * 2.0f * std::cosf(diffAngle);
		rushDistance *= rushDistanceRatio_;
		endRushPos_ = commonData_.transform.translate + targetDir * rushDistance;
		endRushPos_.y = 2.0f;

		// アニメーション
		commonData_.animator->StartAnimation(BossAnimationType::kRush, "Rush_End", rushMaxTime_, false);
	}
}

void BossRushAttackAction::RushAttack() {
	timer_ += FpsCounter::gameDeltaTime / rushMaxTime_;

	// 移動
	commonData_.transform.translate = Lerp(startRushPos_, endRushPos_,timer_,EaseType::kEaseOutQuad);

	// 高さ
	if (timer_ <= 0.3f) {
		float localT = timer_ / 0.3f;
		commonData_.transform.translate.y = Lerp(startRushPos_.y, 0.0f, localT,EaseType::kEaseOutQuad);
	} else if (timer_ >= 0.7f) {
		float localT = (timer_ - 0.7f) / 0.3f;
		commonData_.transform.translate.y = Lerp(0.0f, endRushPos_.y, localT,EaseType::kEaseInQuad);
	}

	// 回転
	Vector3 dir = endRushPos_ - startRushPos_;
	dir.Normalize();
	commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);
	commonData_.transform.rotate.z = 0.0f;

	// 突進エフェクト
	commonData_.rangedAttackManager->SetRush(commonData_.transform.translate + (dir * 4.0f), dir, true);

	if (timer_ >= 1.0f) {
		isFinished_ = true;
		commonData_.requestState = BossBattleState::kResetMove;

		// 突進エフェクトの無効化
		commonData_.rangedAttackManager->SetRush(commonData_.transform.translate, dir, false);
	}
}

//===============================================================
// 待機
//===============================================================

BossWaitAction::BossWaitAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void BossWaitAction::Initialize() {
	timer_ = 0.0f;
	isFinished_ = false;

	commonData_.animator->StartAnimation(BossAnimationType::kMove, "基本移動");
}

void BossWaitAction::Update() {
	timer_ += FpsCounter::gameDeltaTime / maxTime_;

	if (timer_ >= 1.0f) {
		isFinished_ = true;
	}
}

void BossWaitAction::Finalize() {

}

void BossWaitAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "Wait";
	int index = 0;

	param->Register("MaxTime", maxTime_, index++, subGroup);
}

//===============================================================
// 横断移動
//===============================================================

BossCrossMoveAction::BossCrossMoveAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void BossCrossMoveAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;
	// 開始位置
	startPos_ = commonData_.transform.translate;
	Vector3 startDir = startPos_;
	startDir.y = 0.0f;
	startDir.Normalize();

	float angle = RandomGenerator::Get(-std::numbers::pi_v<float> / 4.0f, std::numbers::pi_v<float> / 4.0f);
	Vector3 dir = RotateVectorXZ(startDir, angle);
	// 反転
	dir *= -1.0f;

	// 終盤の位置を取得
	endPos_ = dir * (commonData_.stageRadius * crossEndRatio_);

	// 現在の向いている方向を求める
	startCurrentRotDir_ = Math::YawToDirection(commonData_.transform.rotate.y);
	startCurrentRotDir_.Normalize();
	// 最初の内の最後に向く方向
	endRotDir_ = Vector3::Normalize(endPos_);

	// 最終的に向く方向
	finalRotDir_ = endRotDir_ * -1.0f;

	commonData_.animator->StartAnimation(BossAnimationType::kMove, "基本移動");
}

void BossCrossMoveAction::Update() {

	timer_ += FpsCounter::gameDeltaTime / maxTime_;
	timer_ = std::min(timer_, 1.0f);

	// 縦移動
	float posY = 0.0f;
	float totalCycle = timer_ * upDownCount_;
	float localTimer = std::fmodf(totalCycle, 1.0f);
	if (localTimer <= 0.5f) {
		float t = localTimer / 0.5f;
		posY = Lerp(0.0f, maxMoveHeight_, t, EaseType::kEaseInOutQuad);
	} else {
		float t = (localTimer - 0.5f) / 0.5f;
		posY = Lerp(maxMoveHeight_, 0.0f, t, EaseType::kEaseInOutQuad);
	}

	// 移動
	Vector3 pos = Lerp(startPos_, endPos_, timer_, EaseType::kEaseInOutQuad);
	commonData_.transform.translate = pos;
	commonData_.transform.translate.y = defaultPosY_;
	commonData_.transform.translate.y += posY;

	/// 回転
	Vector3 dir = { 0,0,1 };

	// 回転の処理
	if (timer_ <= 0.2f) {
		float localT = timer_ / 0.2f;
		// 回転
		dir = Slerp(startCurrentRotDir_, endRotDir_, localT, EaseType::kEaseInQuad);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);
	} else if (timer_ >= 0.8f) {

		float localT = (timer_ - 0.8f) / 0.2f;
		// 回転
		dir = Slerp(endRotDir_, finalRotDir_, localT, EaseType::kEaseOutQuad);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);
	}
	
	if (timer_ >= 1.0f) {
		isFinished_ = true;
	}
}

void BossCrossMoveAction::Finalize() {

}

void BossCrossMoveAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "CrossMove";
	int index = 0;

	param->Register("CrossEndRatio", crossEndRatio_, index++, subGroup);
	param->Register("DefaultPosY", defaultPosY_, index++, subGroup);
	param->Register("MaxMoveHeight", maxMoveHeight_, index++, subGroup);
	param->Register("UpDownCount", upDownCount_, index++, subGroup);
	param->Register("MaxTime", maxTime_, index++, subGroup);
}

//===============================================================
// 回転移動
//===============================================================

RotateMoveAction::RotateMoveAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void RotateMoveAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;
	
	Vector3 myDir = commonData_.transform.translate;
	myDir.y = 0.0f;
	myDir.Normalize();
	// 最初の角度
	startAngle_ = std::atan2f(myDir.z, myDir.x);

	// 回転する方向を求める
	float rotOffset = 0.0f;
	if (RandomGenerator::Get(0, 1) == 0) {
		rotOffset = std::numbers::pi_v<float> *0.5f;
	} else {
		rotOffset = -std::numbers::pi_v<float> *0.5f;
	}

	// 反対側の角度を求める
	endAngle_ = startAngle_ + rotOffset;

	// 上下移動する回数を求める
	float radius = commonData_.stageRadius + offsetStageRadius_;
	Vector3 endPos = GetXZFromAngle(endAngle_, radius, defaultPosY_);
	float angleDiff = std::fabs(endPos.Length());
	cycleCount_ = angleDiff / (commonData_.stageRadius * 0.5f);

	// 最初の回転するための角度を求める
	startCurrentRotDir_ = commonData_.transform.translate;
	startCurrentRotDir_.y = 0.0f;
	startCurrentRotDir_.Normalize();
	// 最初の内の最後に向く方向
	float angle = Lerp(startAngle_, endAngle_, 0.2f);
	Vector3 prePos = GetXZFromAngle(angle, radius, defaultPosY_);
	endRotDir_ = Vector3::Normalize(prePos - commonData_.transform.translate);
	// 最終的に向く方向
	angle = Lerp(startAngle_, endAngle_, 1.0f);
	prePos = GetXZFromAngle(angle, radius, defaultPosY_);
	finalRotDir_ = Vector3::Normalize(prePos * -1.0f);

	commonData_.animator->StartAnimation(BossAnimationType::kMove, "基本移動");
}

void RotateMoveAction::Update() {
	timer_ += FpsCounter::gameDeltaTime / maxTime_;
	timer_ = std::min(timer_, 1.0f);

	// 角度補間
	float preAngle = angle_;
	angle_ = Lerp(startAngle_, endAngle_, timer_);

	// 半径を取得
	float radius = commonData_.stageRadius + offsetStageRadius_;

	// 縦移動
	float posY = 0.0f;
	float totalCycle = timer_ * cycleCount_;
	float localTimer = std::fmodf(totalCycle, 1.0f);

	if (localTimer <= 0.5f) {
		float t = localTimer / 0.5f;
		posY = Lerp(0.0f, maxMoveHeight_, t, EaseType::kEaseInOutQuad);
	} else {
		float t = (localTimer - 0.5f) / 0.5f;
		posY = Lerp(maxMoveHeight_, 0.0f, t, EaseType::kEaseInOutQuad);
	}

	// 回転移動
	commonData_.transform.translate = GetXZFromAngle(angle_, radius, defaultPosY_);
	commonData_.transform.translate.y += posY;

	/// 回転
	Vector3 dir = { 0,0,1 };

	// 回転の処理
	if (timer_ <= 0.2f) {
		float localT = timer_ / 0.2f;

		// 回転
		dir = Slerp(startCurrentRotDir_, endRotDir_, localT, EaseType::kEaseInQuad);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

	} else if (timer_ <= 0.8f) {
		// 回転
		Vector3 prePos = GetXZFromAngle(preAngle, radius, defaultPosY_);
		dir = commonData_.transform.translate - prePos;
		dir.Normalize();
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);
		// 保存
		startCurrentRotDir_ = dir;
	} else {
		float localT = (timer_ - 0.8f) / 0.2f;
		// 回転
		dir = Slerp(startCurrentRotDir_, finalRotDir_, localT, EaseType::kEaseOutQuad);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);
	}

	if (timer_ >= 1.0f) {
		isFinished_ = true;
	}
}

void RotateMoveAction::Finalize() {

}

void RotateMoveAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "RotateMove";
	int index = 0;

	param->Register("DefaultPosY", defaultPosY_, index++, subGroup);
	param->Register("OffsetStageRadius", offsetStageRadius_, index++, subGroup);
	param->Register("MaxTime", maxTime_, index++, subGroup);
	param->Register("MaxMoveHeight", maxMoveHeight_, index++, subGroup);
}

//==========================================================================
// 氷柱攻撃
//==========================================================================

IceFallAttackAction::IceFallAttackAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void IceFallAttackAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;

	// 叫びモージョンに以降
	commonData_.animator->StartAnimation(BossAnimationType::kScream, "Scream", maxTime_, false);

	// 半径を求める
	float radius = commonData_.stageRadius * rangeRadiusRatio_;

	// 氷柱攻撃
	commonData_.rangedAttackManager->StartIceFall(radius, minDistance_, iceFallNum_, iceFallMaxNum_, maxIter_);
}

void IceFallAttackAction::Update() {
	timer_ += FpsCounter::gameDeltaTime / maxTime_;


	if (timer_ >= 1.0f) {
		isFinished_ = true;
	}
}

void IceFallAttackAction::Finalize() {

}

void IceFallAttackAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "IceFallAttack";
	int index = 0;

	param->Register("MaxTime", maxTime_, index++, subGroup);
	param->Register("RangeRadiusRatio", rangeRadiusRatio_, index++, subGroup);
	param->Register("MinDistance", minDistance_, index++, subGroup);
	param->Register("IceFallNum", iceFallNum_, index++, subGroup);
	param->Register("IceFallMaxNum", iceFallMaxNum_, index++, subGroup);
	param->Register("MaxIter", maxIter_, index++, subGroup);
}

//==========================================================================
// 風攻撃
//==========================================================================

WindAttackAction::WindAttackAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void WindAttackAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;
	state_ = State::kIn;

	// 現在の方向を求める
	startCurrentRotDir_ = commonData_.transform.translate;
	startCurrentRotDir_.y = 0.0f;
	startCurrentRotDir_.Normalize();
	startCurrentRotDir_ *= -1.0f;

	float angle = std::numbers::pi_v<float> / 4.0f;
	startRotDir_ = RotateVectorXZ(startCurrentRotDir_, angle);
	endRotDir_ = RotateVectorXZ(startCurrentRotDir_, -angle);

	commonData_.animator->StartAnimation(BossAnimationType::kBreath, "IceBreath_Prepare", inMaxTime_,false);
}

void WindAttackAction::Update() {

	switch (state_)
	{
	case WindAttackAction::State::kIn: {
		timer_ += FpsCounter::gameDeltaTime / inMaxTime_;

		// 回転
		Vector3 dir = Slerp(startCurrentRotDir_, startRotDir_, timer_);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

		if (timer_ >= 1.0f) {
			state_ = State::kMain;
			timer_ = 0.0f;

			commonData_.animator->StartAnimation(BossAnimationType::kBreath, "IceBreath_Main", mainMaxTime_, false);

			// 風攻撃を開始
			Vector3 startDir = startRotDir_;
			startDir.y = windDirY_;
			startDir.Normalize();
			Vector3 endDir = endRotDir_;
			endDir.y = windDirY_;
			endDir.Normalize();
			// 風の演出
			commonData_.rangedAttackManager->StartWind(commonData_.transform.translate, startDir, endDir, mainMaxTime_);
		}
		break;
	}

	case WindAttackAction::State::kMain: {
		timer_ += FpsCounter::gameDeltaTime / mainMaxTime_;

		// 回転
		Vector3 dir = Slerp(startRotDir_, endRotDir_, timer_);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

		if (timer_ >= 1.0f) {
			state_ = State::kOut;
			timer_ = 0.0f;
			commonData_.animator->StartAnimation(BossAnimationType::kMove, "基本移動");
		}
		break;
	}


	case WindAttackAction::State::kOut: {
		timer_ += FpsCounter::gameDeltaTime / outMaxTime_;

		// 回転
		Vector3 dir = Slerp(endRotDir_, startCurrentRotDir_, timer_);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

		if (timer_ >= 1.0f) {
			isFinished_ = true;
		}
		break;
	}
	}
}

void WindAttackAction::Finalize() {

}

void WindAttackAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "WindAttack";
	int index = 0;
	
	param->Register("InMaxTime", inMaxTime_, index++, subGroup);
	param->Register("MainMaxTime", mainMaxTime_, index++, subGroup);
	param->Register("OutMaxTime", outMaxTime_, index++, subGroup);
	param->Register("WindDirY", windDirY_, index++, subGroup);
}

//================================================================================
// 位置と方向をリセット
//================================================================================

ResetAction::ResetAction(BossBattleStateCommonData& commonData) : IBossBattleAction(commonData) {

}

void ResetAction::Initialize() {
	isFinished_ = false;
	timer_ = 0.0f;
	state_ = State::kIn;

	// 最初の位置を求める
	startPos_ = commonData_.transform.translate;

	// ステージの中心からのベクトルを求める
	Vector3 dir = commonData_.transform.translate;
	if (commonData_.transform.translate.x == 0.0f && commonData_.transform.translate.z == 0.0f) {
		dir = { 0.0f,0.0f,1.0f };
	}

	// 移動時間を求める
	moveMaxTime_ = dir.Length() / moveSpeed_;

	// ベクトルを正規化
	dir.y = 0.0f;
	dir.Normalize();

	// 最後の位置を求める
	endPos_ = dir * (commonData_.stageRadius * 0.8f);
	endPos_.y = defaultPosY_;

	// 現在の向いている方向を求める
	inStartRotDir_ = Math::YawToDirection(commonData_.transform.rotate.y);

	// 進む方向を求める
	inEndRotDir_ = endPos_ - startPos_;
	inEndRotDir_.y = 0.0;
	inEndRotDir_.Normalize();

	// 回転する時価を求める
	float angle = Math::AngleBetweenRadians(inStartRotDir_, inEndRotDir_);
	if (angle > 0.0001f) {
		inRotateMaxTime_ = angle / rotateSpeed_;
	} else {
		inRotateMaxTime_ = 0.0001f;
	}

	// 最後
	outStartRotDir_ = inEndRotDir_;
	outEndRotDir_ = endPos_ * -1.0f;
	outEndRotDir_.y = 0.0f;
	outEndRotDir_.Normalize();

	// 回転する時価を求める
	angle = Math::AngleBetweenRadians(outStartRotDir_, outEndRotDir_);
	if (angle > 0.0001f) {
		outRotateMaxTime_ = angle / rotateSpeed_;
	} else {
		outRotateMaxTime_ = 0.0001f;
	}

	commonData_.animator->StartAnimation(BossAnimationType::kMove, "基本移動");
}

void ResetAction::Update() {

	switch (state_)
	{
	case ResetAction::State::kIn: {
		timer_ += FpsCounter::gameDeltaTime / inRotateMaxTime_;

		// 回転
		Vector3 dir = Slerp(inStartRotDir_, inEndRotDir_, timer_);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

		if (timer_ >= 1.0f) {
			commonData_.transform.rotate.y = std::atan2f(inEndRotDir_.x, inEndRotDir_.z);
			timer_ = 0.0f;
			state_ = State::kMain;
		}
		break;
	}

	case ResetAction::State::kMain: {
		timer_ += FpsCounter::gameDeltaTime / moveMaxTime_;

		// 移動
		commonData_.transform.translate = Lerp(startPos_, endPos_, timer_, EaseType::kEaseInOutQuad);

		if (timer_ >= 1.0f) {
			timer_ = 0.0f;
			state_ = State::kOut;
			commonData_.transform.translate = endPos_;
		}
		break;
	}

	case ResetAction::State::kOut: {
		timer_ += FpsCounter::gameDeltaTime / outRotateMaxTime_;

		// 回転
		Vector3 dir = Slerp(outStartRotDir_, outEndRotDir_, timer_);
		// Y軸周りの角度
		commonData_.transform.rotate.y = std::atan2f(dir.x, dir.z);

		if (timer_ >= 1.0f) {
			commonData_.transform.translate = endPos_;
			isFinished_ = true;
		}
		break;
	}
	}

}

void ResetAction::Finalize() {

}

void ResetAction::RegisterParameter(GameEngine::DebugParameter* param) {
	std::string subGroup = "ResetAction";
	int index = 0;

	param->Register("MoveSpeed", moveSpeed_, index++, subGroup);
	param->Register("RotateSpeed", rotateSpeed_, index++, subGroup);
}

// ヘルパー関数
namespace {

	Vector3 GetXZFromAngle(float angle, float radius, float posY) {
		return  Vector3(std::cosf(angle) * radius, posY, std::sinf(angle) * radius);
	}

	Vector3 RotateVectorXZ(Vector3 dir, float angle) {
		float cos = std::cosf(angle);
		float sin = std::sinf(angle);
		return Vector3(dir.x * cos - dir.z * sin, 0.0f, dir.x * sin + dir.z * cos );
	}
}




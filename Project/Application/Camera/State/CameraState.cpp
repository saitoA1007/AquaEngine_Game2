#include "pch.h"
#include "CameraState.h"
#include "Application/Camera/CameraController.h"
#include "EasingManager.h"
#include "FPSCounter.h"
#include "DebugParameter.h"
using namespace GameEngine;

//======================================================================
// 追跡カメラ
//======================================================================

FollowCameraState::FollowCameraState(CameraController* controller, GameEngine::DebugParameter* param) : ICameraState(controller) {
	std::string subGroup = "Follow";
	int index = 0;
	param->Register("PositionLerpRate", commonData_.positionLerpRate, index++, subGroup);
	param->Register("TargetLerpRate", commonData_.targetLerpRate, index++, subGroup);
	param->Register("OffsetY", offsetY_, index++, subGroup);
	param->Register("Distance", kDistance_, index++, subGroup);
	param->Register("RotateSpeed", kRotateSpeed_, index++, subGroup);
	param->Register("RotateDamping", kRotateDamping_, index++, subGroup);
	param->Register("RotateY", kFollowRotateY_, index++, subGroup);
	param->Register("Fov", kFollowFov_, index++, subGroup);
}

void FollowCameraState::Enter() {
	// ロックオン解除時は慣性をリセットして違和感なく戻す
	rotateVelocityX_ = 0.0f;
}

void FollowCameraState::Update(float dt60) {

	// カメラの切り替え処理
	if (controller_->GetInputCommand()->IsCommandActive("CameraLockOn")) {
		commonData_.requestState = CameraState::kLockOn;
	}

	Vector2& rotateMove = controller_->GetRotateMove();

	Vector3 idealTarget = controller_->GetPlayerWorld()->GetWorldPosition();
	idealTarget.y += offsetY_;

	float targetRotateSpeed = 0.0f;
	if (controller_->GetInputCommand()->IsCommandActive("CameraMoveLeft")) {
		targetRotateSpeed += kRotateSpeed_;
	}

	if (controller_->GetInputCommand()->IsCommandActive("CameraMoveRight")) {
		targetRotateSpeed -= kRotateSpeed_;
	}

	float currentDamping = std::powf(kRotateDamping_, dt60);
	rotateVelocityX_ = rotateVelocityX_ * currentDamping + targetRotateSpeed * (1.0f - currentDamping);
	rotateMove.x += rotateVelocityX_ * FpsCounter::gameDeltaTime;
	rotateMove.y = kFollowRotateY_;

	Vector3 idealPosition;
	float distance = kDistance_;
	idealPosition.x = idealTarget.x + distance * std::sinf(rotateMove.y) * std::sinf(rotateMove.x);
	idealPosition.y = idealTarget.y + distance * std::cosf(rotateMove.y);
	idealPosition.z = idealTarget.z + distance * std::sinf(rotateMove.y) * std::cosf(rotateMove.x);

	commonData_.idealTarget = idealTarget;
	commonData_.idealPosition = idealPosition;
	commonData_.targetFov = kFollowFov_;
}

//======================================================================
// ロックオンカメラ
//======================================================================

LockOnCameraState::LockOnCameraState(CameraController* controller, GameEngine::DebugParameter* param) : ICameraState(controller) {
	std::string subGroup = "LockOn";
	int index = 0;
	param->Register("PositionLerpRate", commonData_.positionLerpRate, index++, subGroup);
	param->Register("TargetLerpRate", commonData_.targetLerpRate, index++, subGroup);
	param->Register("FovLerpRate", commonData_.fovLerpRate, index++, subGroup);
	param->Register("LockOnRotateRate", lockOnRotateRate_, index++, subGroup);
	param->Register("MinLockOnDistance", kMinLockOnDistance_, index++, subGroup);
	param->Register("LockOnPlayerOffsetY", lockOnPlayerOffsetY_, index++, subGroup);
	param->Register("LockOnTargetOffsetY", lockOnTargetOffsetY_, index++, subGroup);
	param->Register("LockOnNearFov", kLockOnNearFov_, index++, subGroup);
	param->Register("LockOnFarFov", kLockOnFarFov_, index++, subGroup);
	param->Register("LockOnFovMinDist", kLockOnFovMinDist_, index++, subGroup);
	param->Register("LockOnFovMaxDist", kLockOnFovMaxDist_, index++, subGroup);
}

void LockOnCameraState::Update(float dt60) {

	// カメラの切り替え処理
	if (controller_->GetInputCommand()->IsCommandActive("CameraLockOn")) {
		commonData_.requestState = CameraState::kFollow;
	}

	Vector2& rotateMove = controller_->GetRotateMove();

	Vector3 playerPos = controller_->GetPlayerWorld()->transform_.translate;
	Vector3 enemyPos = controller_->GetTargetWorld()->transform_.translate;

	Vector3 idealTarget = (playerPos + enemyPos) * 0.5f;
	idealTarget.y += lockOnPlayerOffsetY_;

	Vector3 dir = playerPos - enemyPos;
	float heightDiff = dir.y;
	dir.y = 0.0f;

	float dist = dir.Length();
	if (dist > 0.001f) {
		dir.x /= dist;
		dir.z /= dist;
	} else {
		dir = { 0.0f, 0.0f, 1.0f };
	}

	float t = (dist - kLockOnFovMinDist_) / (kLockOnFovMaxDist_ - kLockOnFovMinDist_);
	t = std::clamp(t, 0.0f, 1.0f);
	float targetFov = Lerp(kLockOnNearFov_, kLockOnFarFov_, t);

	float currentDistance = kMinLockOnDistance_ + dist * 0.6f;

	float targetAngleX = std::atan2f(-dir.x, -dir.z);
	float angleDiff = Math::GetAngleDiff(rotateMove.x, targetAngleX);
	float actualAngleLerp = 1.0f - std::powf(1.0f - lockOnRotateRate_, dt60);
	rotateMove.x += angleDiff * actualAngleLerp;

	Vector3 idealPosition;
	idealPosition.x = idealTarget.x - std::sinf(rotateMove.x) * currentDistance;
	idealPosition.z = idealTarget.z - std::cosf(rotateMove.x) * currentDistance;
	idealPosition.y = playerPos.y + lockOnTargetOffsetY_ + (dist * 0.1f) + (heightDiff * 1.0f);

	commonData_.idealTarget = idealTarget;
	commonData_.idealPosition = idealPosition;
	commonData_.targetFov = targetFov;
}

//=================================================================================
// 入りのムービーカメラ
//=================================================================================

EnterMovieCameraState::EnterMovieCameraState(CameraController* controller, GameEngine::DebugParameter* param) : ICameraState(controller) {
	std::string subGroup = "EnterMovie";
	int index = 0;
	param->Register("PositionLerpRate", commonData_.positionLerpRate, index++, subGroup);
	param->Register("TargetLerpRate", commonData_.targetLerpRate, index++, subGroup);
	param->Register("MovePosition", kMovePos_, index++, subGroup);
	param->Register("MoveOffsetY", kMoveOfferY_, index++, subGroup);
	param->Register("WaitPosition", kWaitPos_, index++, subGroup);
	param->Register("MoveFov", kMoveFov_, index++, subGroup);
	param->Register("WaitFov", kWaitFov_, index++, subGroup);
	param->Register("MoveMaxTime", kMoveMaxTime_, index++, subGroup);
	param->Register("WaitMaxTime", kWaitMaxTime_, index++, subGroup);
}

void EnterMovieCameraState::Enter() {
	phase_ = Phase::kMove;
	timer_ = 0.0f;
}

void EnterMovieCameraState::Update(float dt60) {

	Vector3 enemyPos = controller_->GetTargetWorld()->transform_.translate;
	commonData_.idealTarget = enemyPos;

	switch (phase_)
	{
	case EnterMovieCameraState::Phase::kMove:
		
		timer_ += FpsCounter::gameDeltaTime / kMoveMaxTime_;

		commonData_.idealTarget.y += kMoveOfferY_;
		commonData_.idealPosition = kMovePos_;
		commonData_.targetFov = kMoveFov_;

		if (timer_ >= 1.0f) {
			timer_ = 0.0f;
			phase_ = Phase::kWait;
		}
		break;


	case EnterMovieCameraState::Phase::kWait:
		timer_ += FpsCounter::gameDeltaTime / kWaitMaxTime_;

		commonData_.idealPosition = kWaitPos_;
		commonData_.targetFov = kWaitFov_;

		if (timer_ >= 1.0f) {

		}
		break;
	}
}

//=================================================================================
// クリアのムービーカメラ
//=================================================================================

ClearMovieCameraState::ClearMovieCameraState(CameraController* controller, GameEngine::DebugParameter* param) : ICameraState(controller) {
	std::string subGroup = "ClearMovie";
	int index = 0;
	param->Register("PositionLerpRate", commonData_.positionLerpRate, index++, subGroup);
	param->Register("TargetLerpRate", commonData_.targetLerpRate, index++, subGroup);
	
}

void ClearMovieCameraState::Enter() {

}

void ClearMovieCameraState::Update(float dt60) {

}

//================================================================================
// タイトルのカメラ
//================================================================================

TitleCameraState::TitleCameraState(CameraController* controller, GameEngine::DebugParameter* param) : ICameraState(controller) {
	std::string subGroup = "Title";
	int index = 0;
	param->Register("PositionLerpRate", commonData_.positionLerpRate, index++, subGroup);
	param->Register("TargetLerpRate", commonData_.targetLerpRate, index++, subGroup);
	param->Register("IdealTarget", commonData_.idealTarget, index++, subGroup);
	param->Register("IdealPosition", commonData_.idealPosition, index++, subGroup);
	param->Register("Fov", commonData_.targetFov, index++, subGroup);
}

void TitleCameraState::Enter() {

}

void TitleCameraState::Update(float dt60) {

}

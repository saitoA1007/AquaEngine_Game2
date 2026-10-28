#include "pch.h"
#define NOMINMAX
#include "CameraController.h"
#include "MyMath.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "State/CameraState.h"
using namespace GameEngine;

CameraController::CameraController(GameEngine::InputCommand* inputCommand, const GameEngine::WorldTransform* targetWorld, const GameEngine::WorldTransform* playerWorld) {
	inputCommand_ = inputCommand;
	targetWorld_ = targetWorld;
	playerWorld_ = playerWorld;

	// カメラの初期化
	camera_ = std::make_unique<Camera>();
	camera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},position_ }, 1280, 720);

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Camera");
	
	// 登録
	states_[CameraState::kFollow] = std::make_unique<FollowCameraState>(this, debugParame_.get());
	states_[CameraState::kLockOn] = std::make_unique<LockOnCameraState>(this, debugParame_.get());
	states_[CameraState::kTitle] = std::make_unique<TitleCameraState>(this, debugParame_.get());
	states_[CameraState::kEnterMovie] = std::make_unique<EnterMovieCameraState>(this, debugParame_.get());
	states_[CameraState::kClearMovie] = std::make_unique<ClearMovieCameraState>(this, debugParame_.get());

	// パラメーターの値を適応
	debugParame_->Apply();

	currentStateType_ = CameraState::kFollow;
	currentState_ = states_[currentStateType_].get();
	currentState_->Enter();
}

void CameraController::Initialize() {
	// カメラの位置を初期化
	camera_->transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,4.0f,-10.0f } };

	currentTarget_ = targetWorld_->GetWorldPosition();
	currentTarget_.y = 1.0f;

	currentStateType_ = CameraState::kFollow;
	currentState_ = states_[currentStateType_].get();
	currentState_->Enter();
}

void CameraController::Update() {
	debugParame_->ApplyIfDirty();
	
	float dt60 = FpsCounter::deltaTime * FpsCounter::maxFrameCount;
	
	// カメラを切り替える
	CameraCommonData& common = currentState_->GetCommonData();

	// 内部からの切り替え
	if (common.requestState.has_value()) {
		ChangeState(common.requestState.value());
		common.requestState = std::nullopt;
	}
	// 外部からの切り替え
	if (requestState.has_value()) {
		ChangeState(requestState.value());
		requestState = std::nullopt;
	}

	// 更新
	currentState_->Update(dt60);
	
	// 補間
	float actualTargetLerp = 1.0f - std::powf(1.0f - common.targetLerpRate, dt60);
	float actualPositionLerp = 1.0f - std::powf(1.0f - common.positionLerpRate, dt60);
	currentTarget_ = Lerp(currentTarget_, common.idealTarget, actualTargetLerp);
	position_ = Lerp(position_, common.idealPosition, actualPositionLerp);
	// Fovを補間
	targetFov_ = common.targetFov;
	float actualFovLerp = 1.0f - std::powf(1.0f - common.fovLerpRate, dt60);
	currentFov_ = currentFov_ + (targetFov_ - currentFov_) * actualFovLerp;
	camera_->SetProjectionMatrix(currentFov_, 1280, 720, 0.1f, 200.0f);
	
	// 回転行列に変換
	Matrix4x4 rotateMatrix_ = LookAt(position_, currentTarget_, { 0.0f,1.0f,0.0f });
	// ワールド行列
	Matrix4x4 worldMatrix_ = rotateMatrix_;
	worldMatrix_.m[3][0] = position_.x;
	worldMatrix_.m[3][1] = position_.y;
	worldMatrix_.m[3][2] = position_.z;
	
	// ワールド行を設定
	camera_->SetWorldMatrix(worldMatrix_);
	camera_->UpdateFromWorldMatrix();
}

void CameraController::ChangeState(CameraState next) {
	currentState_->Exit();
	currentStateType_ = next;
	currentState_ = states_[currentStateType_].get();
	currentState_->Enter();
}

Matrix4x4 CameraController::LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
	Vector3 f = Vector3::Normalize(center - eye); // 前方向ベクトル
	Vector3 s = Vector3::Normalize(up.Cross(f)); // 右方向ベクトル
	Vector3 u = f.Cross(s); // 上方向ベクトル

	Matrix4x4 result = { {
		{ s.x,  s.y, s.z, 0 },
		{ u.x,  u.y, u.z, 0 },
		{ f.x,  f.y, f.z, 0 },
		{ 0.0f, 0.0f, 0.0f, 1}
	} };
	return result;
}
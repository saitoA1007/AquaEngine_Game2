#include "pch.h"
#include "FructureDemo.h"
#include "FPSCounter.h"
#include "Application/CollisionConfig.h"
using namespace GameEngine;

FructureDemo::FructureDemo(std::string name, GameEngine::InputCommand* inputCommand, GameEngine::Model* model) :
	destructibleObject_(name, model, static_cast<uint32_t>(CollisionTypeID::kPlayer), kCollisionAttributePlayer) {

	// 入力機能を取得
	inputCommand_ = inputCommand;

	// 当たり判定を設定
	testCollider_.SetRadius(1.0f);
	testCollider_.SetWorldPosition(testPos_);
	testCollider_.SetCollisionAttribute(kCollisionAttributeEnemy);
	testCollider_.SetCollisionMask(~kCollisionAttributeEnemy);

	std::string subGroup = "Material";
	int index = 0;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>(name);
	debugParame_->Register("IceColor", iceMaterial_.materialData_->color, index++, subGroup);
	debugParame_->Register("IceRoughness", iceMaterial_.materialData_->roughness, index++, subGroup);
	debugParame_->Register("IceIor", iceMaterial_.materialData_->ior, index++, subGroup);
	debugParame_->RegisterWorld("", destructibleObject_.worldTransform_);

	subGroup = "Material/Surface";
	debugParame_->Register("IceChipScale", iceMaterial_.materialData_->chipScale, index++, subGroup);
	debugParame_->Register("IceChipStrength", iceMaterial_.materialData_->chipStrength, index++, subGroup);
	debugParame_->Register("IceEdgeWidth", iceMaterial_.materialData_->edgeWidth, index++, subGroup);
	debugParame_->Register("IceEdgeStrength", iceMaterial_.materialData_->edgeStrength, index++, subGroup);
	debugParame_->Register("IceMicroScale", iceMaterial_.materialData_->microScale, index++, subGroup);
	debugParame_->Register("IceMicroStrength", iceMaterial_.materialData_->microStrength, index++, subGroup);
	debugParame_->Register("IceDissolveThreshold", iceMaterial_.materialData_->dissolveThreshold, index++, subGroup);
	subGroup = "Material/Bubble";
	index = 0;
	debugParame_->Register("bubbleScale", iceMaterial_.materialData_->bubbleScale, index++, subGroup);
	debugParame_->Register("bubbleMaxDepth", iceMaterial_.materialData_->bubbleMaxDepth, index++, subGroup);
	debugParame_->Register("bubbleDensity", iceMaterial_.materialData_->bubbleDensity, index++, subGroup);
	debugParame_->Register("bubbleJitter", iceMaterial_.materialData_->bubbleJitter, index++, subGroup);
	debugParame_->Register("bubbleHighlight", iceMaterial_.materialData_->bubbleHighlight, index++, subGroup);
	subGroup = "TestCollider";
	index = 0;
	debugParame_->Register("Radius", radius_, index++, subGroup);
	debugParame_->Apply();
}

void FructureDemo::Initialize() {

}

void FructureDemo::Update() {
	debugParame_->ApplyIfDirty();

	// 移動
	Move();

	// 破片の更新
	destructibleObject_.Update();
}

void FructureDemo::DebugUpdate() {
	Update();
}

void FructureDemo::Draw() {
	// 破片を描画
	destructibleObject_.Draw();
}

void FructureDemo::Move() {

	// カメラ更新
	UpdateCameraBasis();

	Vector3 dir = { 0,0,0 };
	// XZの目標速度
	Vector3 desiredVelocityXZ = { 0,0,0 };

	// 移動の操作
	if (inputCommand_->IsCommandActive("MoveForward")) { dir -= cameraForwardXZ_; }
	if (inputCommand_->IsCommandActive("MoveBack")) { dir += cameraForwardXZ_; }
	if (inputCommand_->IsCommandActive("MoveLeft")) { dir -= cameraRightXZ_; }
	if (inputCommand_->IsCommandActive("MoveRight")) { dir += cameraRightXZ_; }

	// 目標方向
	if (dir.x != 0.0f || dir.z != 0.0f) {
		dir.y = 0.0f;
		dir.Normalize();
		// 目標速度を設定
		desiredVelocityXZ = dir * 1.0f;
	}

	testPos_ += desiredVelocityXZ * (5.0f * FpsCounter::gameDeltaTime);

	// 当たり判定を更新
	testCollider_.SetWorldPosition(testPos_);
	testCollider_.SetRadius(radius_);

	// 破壊オブジェクトを元の姿へ戻す
	if (inputCommand_->IsCommandActive("Reassemble")) {
		// リセット
		destructibleObject_.Reassemble();
	}

}

void FructureDemo::UpdateCameraBasis() {

	Matrix4x4 cameraWorldMatrix = renderQueue_->GetDebugCameraWorldMatrix();

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
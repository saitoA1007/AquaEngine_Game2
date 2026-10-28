#include "pch.h"
#include"Camera.h"
#include"EngineSource/Math/MyMath.h"
#include"EngineSource/Common/CreateBufferResource.h"

using namespace GameEngine;

Camera::~Camera() {
	
}

void Camera::Initialize(const Transform& transform, int kClientWidth, int kClientHeight) {
	// Matrixの初期化
	transform_ = transform;
	worldMatrix_ = Matrix4x4::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	viewMatrix_ = Matrix4x4::Inverse(worldMatrix_);
	projectionMatrix_ = Matrix4x4::MakePerspectiveFovMatrix(0.45f, static_cast<float>(kClientWidth) / static_cast<float>(kClientHeight), 0.1f, 200.0f);
	VPMatrix_ = viewMatrix_ * projectionMatrix_;


	// 定数バッファの作成
	constBuffer_.Create();
	cameraForGPU_ = constBuffer_.GetData();
	// 単位行列を書き込んでおく
	cameraForGPU_->worldPosition = GetWorldPosition();
	cameraForGPU_->vpMatrix = Matrix4x4::MakeIdentity();
	cameraForGPU_->mtxViewInv = Matrix4x4::MakeIdentity();
	cameraForGPU_->mtxProjInv = Matrix4x4::MakeIdentity();
	cameraForGPU_->viewMatrix = Matrix4x4::MakeIdentity();
	cameraForGPU_->projectionMatrix = Matrix4x4::MakeIdentity();
}

void Camera::Update() {
	worldMatrix_ = Matrix4x4::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	viewMatrix_ = Matrix4x4::Inverse(worldMatrix_);
	VPMatrix_ = viewMatrix_ * projectionMatrix_;

	if (cameraForGPU_) {
		cameraForGPU_->worldPosition = GetWorldPosition();
		cameraForGPU_->vpMatrix = VPMatrix_;
		cameraForGPU_->mtxViewInv = worldMatrix_;
		cameraForGPU_->mtxProjInv = Matrix4x4::Inverse(projectionMatrix_);
		cameraForGPU_->viewMatrix = viewMatrix_;
		cameraForGPU_->projectionMatrix = projectionMatrix_;
	}	
}

void Camera::UpdateFromWorldMatrix() {
	viewMatrix_ = Matrix4x4::Inverse(worldMatrix_);
	VPMatrix_ = viewMatrix_ * projectionMatrix_;

	if (cameraForGPU_) {
		cameraForGPU_->worldPosition = GetWorldPosition();
		cameraForGPU_->vpMatrix = VPMatrix_;
		cameraForGPU_->mtxViewInv = worldMatrix_;
		cameraForGPU_->mtxProjInv = Matrix4x4::Inverse(projectionMatrix_);
		cameraForGPU_->viewMatrix = viewMatrix_;
		cameraForGPU_->projectionMatrix = projectionMatrix_;
	}
}

Matrix4x4 Camera::MakeWVPMatrix(Matrix4x4 worldMatrix) {
	WVPMatrix_ = worldMatrix * (viewMatrix_ * projectionMatrix_);
	return WVPMatrix_;
}

void Camera::SetProjectionMatrix(float fovY, int kClientWidth, int kClientHeight, float nearPlane, float farPlane) {
	this->projectionMatrix_ = Matrix4x4::MakePerspectiveFovMatrix(fovY, static_cast<float>(kClientWidth) / static_cast<float>(kClientHeight), nearPlane, farPlane);
}

void Camera::SetViewMatrix(const Matrix4x4& viewMatrix) {
	this->viewMatrix_ = viewMatrix;
}

Vector3 Camera::GetWorldPosition() const {
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得
	worldPos.x = worldMatrix_.m[3][0];
	worldPos.y = worldMatrix_.m[3][1];
	worldPos.z = worldMatrix_.m[3][2];
	return worldPos;
}

void Camera::SetCamera(const Camera& camera) {
	transform_ = camera.transform_;
	worldMatrix_ = camera.GetWorldMatrix();
	projectionMatrix_ = camera.GetProjectionMatrix();
	viewMatrix_ = camera.GetViewMatrix();
	VPMatrix_ = camera.GetVPMatrix();

	if (cameraForGPU_) {
		cameraForGPU_->worldPosition = camera.GetWorldPosition();
		cameraForGPU_->vpMatrix = VPMatrix_;
		cameraForGPU_->mtxViewInv = worldMatrix_;
		cameraForGPU_->mtxProjInv = Matrix4x4::Inverse(camera.GetProjectionMatrix());
		cameraForGPU_->viewMatrix = camera.GetViewMatrix();
		cameraForGPU_->projectionMatrix = camera.GetProjectionMatrix();
	}
}
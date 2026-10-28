#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"
#include "TransformationMatrix.h"
#include "ConstantBuffer.h"
#include "InPut.h"

namespace GameEngine {

	class DebugCamera {
	public:
		DebugCamera(Input* input);
		~DebugCamera();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="translate">カメラ座標</param>
		/// <param name="width">画面横幅</param>
		/// <param name="height">画面縦幅</param>
		void Initialize(const Vector3& translate, int width, int height);

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update();

	public:

		Matrix4x4 GetVPMatrix();

		Matrix4x4 GetViewMatrix() { return viewMatrix_; }

		Matrix4x4 GetProjectionMatrix() { return projectionMatrix_; }

		Matrix4x4 GetRotateMatrix();

		Matrix4x4 GetWorldMatrix() const { return worldMatrix_; }

		Vector3 GetWorldPosition();

		/// <summary>
		/// カメラの注視点の位置を取得
		/// </summary>
		/// <returns></returns>
		Vector3 GetTargetPosition() const { return targetPos_; }

		/// <summary>
		/// カメラをターゲットの方向に向かせる
		/// </summary>
		/// <param name="eye">カメラの位置</param>
		/// <param name="center">ターゲットの位置</param>
		/// <param name="up">向き</param>
		/// <returns></returns>
		Matrix4x4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up);

		ID3D12Resource* GetResource() const { return constBuffer_.GetResource(); }
		ConstantBuffer<CameraForGPU>* GetConstantBuffer() { return &constBuffer_; }

	private:
		Input* input_ = nullptr;

		// 拡縮
		Vector3 scale_ = { 1.0f,1.0f,1.0f };
		// X,Y,Z軸回りのローカル座標角
		Vector3 rotate_ = { 0.0f,0.0f,0.0f };
		// ローカル座標
		Vector3 translate_ = { 0.0f,0.0f,-10.0f };
		// ワールド行列
		Matrix4x4 worldMatrix_;
		// ビュー行列
		Matrix4x4 viewMatrix_;
		// 射影行列
		Matrix4x4 projectionMatrix_;

		// 累積回転行列
		Matrix4x4 rotateMatrix_;

		// マウスの移動量
		Vector2 mouseMove_{3.1f,1.0f};

		// 目標となる座標
		Vector3 targetPos_ = { 0.0f,0.0f,0.0f };

		// 距離
		float distance_ = 40.0f;

		// ターゲットを動かす速度
		static inline const float kTargetSpeed = 0.5f;

		// GPUにカメラの位置を送る用のリソース
		ConstantBuffer<CameraForGPU> constBuffer_;
		CameraForGPU* cameraForGPU_ = nullptr;
	};
}
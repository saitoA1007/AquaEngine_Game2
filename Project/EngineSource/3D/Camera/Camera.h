#pragma once
#include "Matrix4x4.h"
#include "Vector3.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "ConstantBuffer.h"

namespace GameEngine {

	class Camera {
	public:
		Camera() = default;
		~Camera();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="transform">Scale,Rotate,Translate : 各型Vector3</param>
		/// <param name="kClientWidth">画面横幅</param>
		/// <param name="kClientHeight">画面縦幅</param>
		void Initialize(const Transform& transform, int kClientWidth, int kClientHeight);

		/// <summary>
		/// カメラの更新処理
		/// </summary>
		void Update();

		/// <summary>
		/// ワールド行列から更新処理をおこなう
		/// </summary>
		/// <param name="worldMatrix"></param>
		void UpdateFromWorldMatrix();

	public:

		/// <summary>
		/// カメラのワールド位置を取得する
		/// </summary>
		/// <returns></returns>
		Vector3 GetWorldPosition() const;

		/// <summary>
		/// ワールド行列を設定する
		/// </summary>
		/// <param name="worldMatrix"></param>
		void SetWorldMatrix(const Matrix4x4& worldMatrix) { worldMatrix_ = worldMatrix; }

		/// <summary>
		/// ワールド行列を取得
		/// </summary>
		/// <returns></returns>
		Matrix4x4 GetWorldMatrix() const { return worldMatrix_; }

		Matrix4x4 GetViewMatrix() const { return viewMatrix_; }
		void SetViewMatrix(const Matrix4x4& viewMatrix);

		/// <summary>
		/// カメラの描画範囲を設定
		/// </summary>
		/// <param name="fovY">視野</param>
		/// <param name="kClientWidth">画面の横幅</param>
		/// <param name="kClientHeight">縦幅</param>
		/// <param name="nearPlane">描画する最小の距離</param>
		/// <param name="farPlane">描画する最大の距離</param>
		void SetProjectionMatrix(float fovY, int kClientWidth, int kClientHeight, float nearPlane, float farPlane);

		Matrix4x4 GetProjectionMatrix() const { return projectionMatrix_; }

		/// <summary>
		/// ビュープロジェクション行列を設定
		/// </summary>
		/// <param name="VPMatrix"></param>
		void SetVPMatrix(Matrix4x4 VPMatrix) { 
			VPMatrix_ = VPMatrix;
			if (cameraForGPU_) {
				cameraForGPU_->vpMatrix = VPMatrix;
			}
		}

		/// <summary>
		/// ビュープロジェクション行列を取得
		/// </summary>
		/// <returns></returns>
		Matrix4x4 GetVPMatrix() const { return VPMatrix_; }

		/// <summary>
		/// モデルをカメラ座標に変換する処理
		/// </summary>
		/// <param name="worldMatrix"></param>
		/// <returns></returns>
		Matrix4x4 MakeWVPMatrix(Matrix4x4 worldMatrix);

		/// <summary>
		/// 別のカメラの内容を受け取る
		/// </summary>
		/// <param name="camera">別のカメラ</param>
		void SetCamera(const Camera& camera);

		ID3D12Resource* GetResource() const { return constBuffer_.GetResource(); }
		ConstantBuffer<CameraForGPU>* GetConstantBuffer() { return &constBuffer_; }
	public:

		// カメラのトランスフォーム
		Transform transform_;

	private:
		// ワールド行列
		Matrix4x4 worldMatrix_;
		// ビュー行列
		Matrix4x4 viewMatrix_;
		// プロジェクション行列
		Matrix4x4 projectionMatrix_;
		// ビュープロジェクション行列
		Matrix4x4 VPMatrix_;
		Matrix4x4 WVPMatrix_;

		// カメラのリソース
		ConstantBuffer<CameraForGPU> constBuffer_;
		CameraForGPU* cameraForGPU_ = nullptr;
	};
}
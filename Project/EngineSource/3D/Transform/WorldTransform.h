#pragma once
#include "Matrix4x4.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "AnimationData.h"
#include "ConstantBuffer.h"

namespace GameEngine {

	/// <summary>
	/// 単体描画用のワールド行列
	/// </summary>
	class WorldTransform {
	public:
		WorldTransform(const Transform& transform = {{1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f}});
		~WorldTransform();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="transform">Scale,Rotate,Translate : 各型Vector3</param>
		void Initialize(const Transform& transform);

		/// <summary>
		/// SRTを適応
		/// </summary>
		void UpdateTransformMatrix();

		void UpdateWorldMatrix(const Matrix4x4 worldMatrix);

	public:

		/// <summary>
		/// ワールド行列を設定する
		/// </summary>
		/// <param name="worldMatrix"></param>
		void SetWorldMatrix(const Matrix4x4 worldMatrix) { worldMatrix_ = worldMatrix; }

		/// <summary>
		/// ワールド行列を取得する
		/// </summary>
		/// <returns></returns>
		Matrix4x4 GetWorldMatrix() const { return worldMatrix_; }

		/// <summary>
		/// 親を設定
		/// </summary>
		/// <param name="parent"></param>
		void SetParent(const WorldTransform* parent){ parent_ = const_cast<WorldTransform*>(parent); }

		/// <summary>
		/// 親を取得
		/// </summary>
		/// <returns></returns>
		WorldTransform* GetParent() { return parent_; }

		/// <summary>
		/// ワールド座標を取得
		/// </summary>
		/// <returns></returns>
		Vector3 GetWorldPosition() const;

		/// <summary>
		/// WVP行列を作成
		/// </summary>
		/// <param name="VPMatrix"></param>
		void SetWVPMatrix(const Matrix4x4& localMatrix);

		// リソースのアドレスを取得
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return constBuffer_.GetGpuVirtualAddress(); }

	public:

		// SRT要素
		Transform transform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };

	private:
		// コピー禁止
		WorldTransform(const WorldTransform&) = delete;
		WorldTransform& operator=(const WorldTransform&) = delete;

		ConstantBuffer<TransformationMatrix> constBuffer_;

		// リソース
		TransformationMatrix* transformationMatrixData_ = nullptr;

		Matrix4x4 worldMatrix_;

		// 親
		WorldTransform* parent_ = nullptr;
	};
}
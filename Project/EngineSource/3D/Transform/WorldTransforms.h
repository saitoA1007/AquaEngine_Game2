#pragma once
#include <queue>
#include <unordered_set>
#include "Matrix4x4.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "StructuredBuffer.h"

namespace GameEngine {

	/// <summary>
	/// 複数描画用のワールド行列
	/// </summary>
	class WorldTransforms {
	public:

		// 1つのパーティクルがもつデータ
		struct TransformData {
			Matrix4x4 worldMatrix;
			Transform transform;
			Vector4 color;
			uint32_t textureHandle;
		};

	public:
		WorldTransforms() = default;
		~WorldTransforms();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="transform"></param>
		void Initialize(const uint32_t& kNumInstance, const Transform& transform);

		/// <summary>
		/// SRTを適応
		/// </summary>
		void UpdateTransformMatrix(const uint32_t& numInstance);

		const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetInstancingSrvGPU() const { return buffer_.GetSrvGpuHandle(); }

		/// <summary>
		/// 描画するモデルの数
		/// </summary>
		/// <returns></returns>
		const uint32_t GetNumInstance() { return numInstance_; }
	public:

		// 各要素のトランスフォーム
		std::vector<TransformData> transformDatas_;

		/// <summary>
		/// WVP行列を作成
		/// </summary>
		/// <param name="VPMatrix"></param>
		void SetWVPMatrix(const uint32_t& numInstance);

		void SetWVPMatrix(const uint32_t& numInstance, const Matrix4x4& localMatrix);

	private:
		// コピー禁止
		WorldTransforms(const WorldTransforms&) = delete;
		WorldTransforms& operator=(const WorldTransforms&) = delete;

		// インスタンスが持つsrvインデックス
		uint32_t srvIndex_ = 0;

		// transformData配列数
		uint32_t numInstance_ = 0;

		// リソース
		StructuredBuffer<ParticleForGPU> buffer_;
		ParticleForGPU* instancingData_ = nullptr;
	};
}


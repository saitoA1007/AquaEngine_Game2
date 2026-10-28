#pragma once
#include "IGameObject.h"
#include "StructuredBuffer.h"
#include "ConstantBuffer.h"
#include "ParticleData.h"

namespace GameEngine {

	// 前方宣言
	class PSOManager;

	// エミッター
	struct EmitterSphere {
		Vector3 translate; // 位置
		float radius;	   // 射出半径
		uint32_t count;	   // 射出数
		float frequency;   // 射出間隔
		float frequencyTime; // 射出間隔調整時間
		uint32_t emit; // 射出許可
	};

	// 時間
	struct PerFrame {
		float time;
		float deltaTime;
	};

	struct FreeListIndex {
		int32_t count;
	};

	struct FreeList {
		uint32_t index;
	};

	class ParticleBehaviorGPU : public IGameObject {
	public:
		ParticleBehaviorGPU(const std::string& name, uint32_t maxNum, Model* model);
		~ParticleBehaviorGPU() = default;

		/// <summary>
		/// 静的初期化
		/// </summary>
		/// <param name="commandList"></param>
		/// <param name="psoManager"></param>
		static void StaticInitialize(ID3D12GraphicsCommandList4* commandList, PSOManager* psoManager);

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		// 描画処理
		void Draw() override;

	public:

		Vector3 emitPos_ = { 0.0f,0.0f,0.0f };

	private:
		static ID3D12GraphicsCommandList4* commandList_;
		static ID3D12RootSignature* emitRootSignature_;
		static ID3D12PipelineState* emitPipelineState_;

		static ID3D12RootSignature* updateRootSignature_;
		static ID3D12PipelineState* updatePipelineState_;

		// パーティクルの名前
		std::string name_;

		Model* model_ = nullptr;

		uint32_t maxNum_ = 0;

		// パーティクルデータ
		StructuredBuffer<ParticleCS> particleBuffer_;

		StructuredBuffer<FreeListIndex> gFreeListIndexBuffer_;

		StructuredBuffer<FreeList> gFreeListBuffer_;

		// エミッター
		ConstantBuffer<EmitterSphere> emitterSphere_;

		// 時間
		ConstantBuffer<PerFrame> perFrame_;

	private:

		void EmitParticleDispatch();

		void UpdateParticleDispatch();

		void UpdateCompute();

	};
}


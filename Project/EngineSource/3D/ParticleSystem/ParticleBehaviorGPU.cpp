#include "pch.h"
#include "ParticleBehaviorGPU.h"
#include "PSOManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

ID3D12GraphicsCommandList4* ParticleBehaviorGPU::commandList_ = nullptr;
ID3D12RootSignature* ParticleBehaviorGPU::emitRootSignature_ = nullptr;
ID3D12PipelineState* ParticleBehaviorGPU::emitPipelineState_ = nullptr;

ID3D12RootSignature* ParticleBehaviorGPU::updateRootSignature_ = nullptr;
ID3D12PipelineState* ParticleBehaviorGPU::updatePipelineState_ = nullptr;

void ParticleBehaviorGPU::StaticInitialize(ID3D12GraphicsCommandList4* commandList, PSOManager* psoManager) {
	commandList_ = commandList;

	auto psoData = psoManager->GetDrawPsoData("EmitComputeParticle");
	emitPipelineState_ = psoData.graphicsPipelineState;
	emitRootSignature_ = psoData.rootSignature;

	auto updatePsoData = psoManager->GetDrawPsoData("UpdateComputeParticle");
	updateRootSignature_ = updatePsoData.rootSignature;
	updatePipelineState_ = updatePsoData.graphicsPipelineState;
}

ParticleBehaviorGPU::ParticleBehaviorGPU(const std::string& name, uint32_t maxNum, Model* model) {

	name_ = name;
	model_ = model;
	maxNum_ = maxNum;

	// パーティクルの数
	std::vector<ParticleCS> particleData;
	particleData.resize(maxNum);

	// パーティクルデータを作成
	particleBuffer_.Create(commandList_, particleData);

	std::vector<FreeListIndex> FreeCounterData;
	FreeCounterData.resize(1);
	FreeCounterData[0].count = maxNum - 1;
	gFreeListIndexBuffer_.Create(commandList_, FreeCounterData);

	std::vector<FreeList> freeListData;
	freeListData.resize(maxNum);
	for (uint32_t i = 0; i < maxNum; ++i) {
		freeListData[i].index = i;
	}
	gFreeListBuffer_.Create(commandList_, freeListData);

	// エミッターを作成
	emitterSphere_.Create();
	auto* emitData = emitterSphere_.GetData();
	emitData->count = 10;
	emitData->frequency = 0.5f;
	emitData->frequencyTime = 0.0f;
	emitData->translate = Vector3(0, 0, 0);
	emitData->radius = 0.5f;
	emitData->emit = 0;

	// 時間を作成
	perFrame_.Create();
	perFrame_.GetData()->deltaTime = 0.0f;
	perFrame_.GetData()->time = 0.0f;
}

void ParticleBehaviorGPU::Initialize() {

}

void ParticleBehaviorGPU::Update() {

	// エミッター
	auto* emitData = emitterSphere_.GetData();
	emitData->frequencyTime += FpsCounter::gameDeltaTime;
	// 射出間隔を上回ったら射出許可を出して時間を調整
	if (emitData->frequency <= emitData->frequencyTime) {
		emitData->frequencyTime -= emitData->frequency;
		emitData->emit = 1;
	} else {
		emitData->emit = 0;
	}

	// 発射位置を更新
	emitData->translate = emitPos_;

	// 時間
	perFrame_.GetData()->deltaTime = FpsCounter::gameDeltaTime;
	perFrame_.GetData()->time += FpsCounter::gameDeltaTime;

	// コンピュートシェーダーを更新
	UpdateCompute();
}

void ParticleBehaviorGPU::Draw() {

	// パーティクルを描画
	renderQueue_->SubmitParticleCS(model_, maxNum_, &particleBuffer_, 0.0f);
}

void ParticleBehaviorGPU::UpdateCompute() {
	// uavに遷移
	particleBuffer_.TransitionUAV(commandList_);

	// エミッター
	EmitParticleDispatch();

	// uavの実行順序を設定
	particleBuffer_.BarrierUAVForUAV(commandList_);

	// 更新
	UpdateParticleDispatch();

	// srvに遷移
	particleBuffer_.TransitionSRV(commandList_);
}

void ParticleBehaviorGPU::EmitParticleDispatch() {
	commandList_->SetComputeRootSignature(emitRootSignature_);
	commandList_->SetPipelineState(emitPipelineState_);

	commandList_->SetComputeRootDescriptorTable(0, particleBuffer_.GetUAVGpuHandle());
	commandList_->SetComputeRootDescriptorTable(1, gFreeListIndexBuffer_.GetUAVGpuHandle());
	commandList_->SetComputeRootDescriptorTable(2, gFreeListBuffer_.GetUAVGpuHandle());
	commandList_->SetComputeRootConstantBufferView(3, emitterSphere_.GetGpuVirtualAddress());
	commandList_->SetComputeRootConstantBufferView(4, perFrame_.GetGpuVirtualAddress());
	commandList_->Dispatch(1, 1, 1);
}

void ParticleBehaviorGPU::UpdateParticleDispatch() {
	commandList_->SetComputeRootSignature(updateRootSignature_);
	commandList_->SetPipelineState(updatePipelineState_);

	commandList_->SetComputeRootDescriptorTable(0, particleBuffer_.GetUAVGpuHandle());
	commandList_->SetComputeRootDescriptorTable(1, gFreeListIndexBuffer_.GetUAVGpuHandle());
	commandList_->SetComputeRootDescriptorTable(2, gFreeListBuffer_.GetUAVGpuHandle());
	commandList_->SetComputeRootConstantBufferView(3, perFrame_.GetGpuVirtualAddress());
	commandList_->Dispatch(1, 1, 1);
}
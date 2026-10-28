#include "pch.h"
#define NOMINMAX
#include <algorithm>
#include "FractureInstance.h"
#include "RuntimeMeshFracturer.h"
#include "MyMath.h"
using namespace GameEngine;

FractureInstance::~FractureInstance() {

}
void FractureInstance::Initialize(const std::vector<uint32_t>& chunkIds, const PackedGeometryBuffer& geometryBuffer) {

	// チャンクの数
	numInstance_ = static_cast<uint32_t>(chunkIds.size());

	// リソース作成
	buffer_.Release();
	buffer_.Create(numInstance_);
	instancingData_ = buffer_.GetData();

	// CPU側のトランスフォーム配列も同じサイズに確保
	transformData_.resize(numInstance_);

	argumentBuffer_.Release();
	argumentBuffer_.Create(numInstance_);
	argumentData_ = argumentBuffer_.GetData();

	for (uint32_t index = 0; index < numInstance_; ++index) {
		// PackedGeometryBufferからこのチャンクの描画範囲を取得
		uint32_t chunkId = chunkIds[index];
		const GeometryRange& range = geometryBuffer.GetRange(chunkId);

		// トランスフォームを初期化
		transformData_[index].transform.scale = { 1.0f, 1.0f, 1.0f };
		transformData_[index].transform.rotate = { 0.0f, 0.0f, 0.0f };
		transformData_[index].transform.translate = { 0.0f, 0.0f, 0.0f };
		instancingData_[index].world = Matrix4x4::MakeIdentity();
		instancingData_[index].worldInverseTranspose = Matrix4x4::MakeIdentity();

		// PackedGeometryBufferから得た描画範囲を設定
		instancingData_[index].vertexOffset = range.vertexOffset;
		instancingData_[index].indexOffset = range.indexOffset;
		instancingData_[index].indexCount = range.indexCount;
		instancingData_[index].chunkId = chunkId;

		// ExecuteIndirect用のDrawコマンド引数を設定
		argumentData_[index].instanceIndex = index;
		argumentData_[index].drawArguments.IndexCountPerInstance = range.indexCount; // 破片のインデックス数
		argumentData_[index].drawArguments.InstanceCount = 1;                        // 1回につき1個描画
		argumentData_[index].drawArguments.StartIndexLocation = range.indexOffset;   // インデックスの開始位置
		argumentData_[index].drawArguments.BaseVertexLocation = static_cast<INT>(range.vertexOffset); // 頂点の開始位置
		argumentData_[index].drawArguments.StartInstanceLocation = 0;
	}
}

void FractureInstance::Update() {
	// 更新
	for (uint32_t i = 0; i < transformData_.size(); ++i) {
		Matrix4x4 localMatrix = Matrix4x4::MakeAffineMatrix(transformData_[i].transform.scale, transformData_[i].transform.rotate, transformData_[i].transform.translate);
		instancingData_[i].world = localMatrix * parentWorldMatrix_;
		instancingData_[i].worldInverseTranspose = Matrix4x4::InverseTranspose(instancingData_[i].world);
	}
}

void FractureInstance::InitializeFromRanges(const std::vector<GeometryRange>& ranges) {
	AllocateBuffers(static_cast<uint32_t>(ranges.size()));
	for (uint32_t index = 0; index < numInstance_; ++index) {
		// ランタイム破片には元のchunkIdがないのでindexを使用
		WriteInstance(index, ranges[index], index);
	}
}

void FractureInstance::ApplyRuntimeCut(const Fragment& source, const Vector3& impactPos, float craterRadius, int numSites, int planeCount) {
	std::vector<Fragment> fragments = RuntimeMeshFracturer::Fracture(
		source, impactPos, craterRadius, numSites, planeCount, kMinTriangleCount);

	runtimeBuffer_ = std::make_unique<RuntimeFractureBuffer>();
	std::vector<GeometryRange> ranges = runtimeBuffer_->Upload(fragments);
	InitializeFromRanges(ranges);
}

void FractureInstance::AllocateBuffers(uint32_t count) {
	numInstance_ = count;

	// 破片が1つも残らなかった場合、0要素でバッファを作るとリソース生成に失敗するため何も持たない状態にする
	if (numInstance_ == 0) {
		buffer_.Release();
		argumentBuffer_.Release();
		instancingData_ = nullptr;
		argumentData_ = nullptr;
		transformData_.clear();
		return;
	}

	buffer_.Release();
	buffer_.Create(numInstance_);
	instancingData_ = buffer_.GetData();

	transformData_.resize(numInstance_);

	argumentBuffer_.Release();
	argumentBuffer_.Create(numInstance_);
	argumentData_ = argumentBuffer_.GetData();
}

void FractureInstance::WriteInstance(uint32_t index, const GeometryRange& range, uint32_t chunkId) {
	transformData_[index].transform.scale = { 1.0f, 1.0f, 1.0f };
	transformData_[index].transform.rotate = { 0.0f, 0.0f, 0.0f };
	transformData_[index].transform.translate = { 0.0f, 0.0f, 0.0f };

	instancingData_[index].world = Matrix4x4::MakeIdentity();
	instancingData_[index].worldInverseTranspose = Matrix4x4::MakeIdentity();
	instancingData_[index].vertexOffset = range.vertexOffset;
	instancingData_[index].indexOffset = range.indexOffset;
	instancingData_[index].indexCount = range.indexCount;
	instancingData_[index].chunkId = chunkId;

	argumentData_[index].instanceIndex = index;
	argumentData_[index].drawArguments.IndexCountPerInstance = range.indexCount;
	argumentData_[index].drawArguments.InstanceCount = 1;
	argumentData_[index].drawArguments.StartIndexLocation = range.indexOffset;
	argumentData_[index].drawArguments.BaseVertexLocation = static_cast<INT>(range.vertexOffset);
	argumentData_[index].drawArguments.StartInstanceLocation = 0;
}

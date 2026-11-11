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

	// チャンクの数だけリソースを確保
	AllocateBuffers(static_cast<uint32_t>(chunkIds.size()));

	for (uint32_t index = 0; index < numInstance_; ++index) {
		// PackedGeometryBufferからこのチャンクの描画範囲を取得
		uint32_t chunkId = chunkIds[index];
		WriteInstance(index, geometryBuffer.GetRange(chunkId), chunkId);
	}
}

void FractureInstance::Update() {
	// 更新
	for (uint32_t i = 0; i < transformData_.size(); ++i) {
		const FractureChunkState& state = transformData_[i];
		// 破片の中心を基準に回転させる
		Matrix4x4 localMatrix = Matrix4x4::MakeTranslateMatrix(state.pivot * -1.0f) *
			Matrix4x4::MakeAffineMatrix(state.transform.scale, state.transform.rotate, state.pivot + state.transform.translate);
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

	// ランタイム破片は重心情報がないので、頂点のAABBから回転の中心と底までの高さを求める
	for (uint32_t index = 0; index < numInstance_ && index < fragments.size(); ++index) {
		const auto& vertices = fragments[index].vertices;
		if (vertices.empty()) { continue; }
		Vector3 min = { vertices[0].position.x, vertices[0].position.y, vertices[0].position.z };
		Vector3 max = min;
		for (const auto& vertex : vertices) {
			min.x = std::min(min.x, vertex.position.x);
			min.y = std::min(min.y, vertex.position.y);
			min.z = std::min(min.z, vertex.position.z);
			max.x = std::max(max.x, vertex.position.x);
			max.y = std::max(max.y, vertex.position.y);
			max.z = std::max(max.z, vertex.position.z);
		}
		Vector3 center = (min + max) * 0.5f;
		SetPivot(index, center, center.y - min.y);
	}
}

void FractureInstance::AllocateBuffers(uint32_t count) {
	numInstance_ = count;

	// 破片が1つも残らなかった場合、0要素でバッファを作るとリソース生成に失敗するため何も持たない状態にする
	if (numInstance_ == 0) {
		buffer_.Release();
		argumentBuffer_.Release();
		instanceIndexBuffer_.Release();
		instanceIndexBufferView_ = {};
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

	// インスタンス番号の頂点バッファ(StartInstanceLocationで各破片の番号を引く)
	instanceIndexBuffer_.Release();
	instanceIndexBuffer_.Create(numInstance_);
	uint32_t* instanceIndexData = instanceIndexBuffer_.GetData();
	for (uint32_t index = 0; index < numInstance_; ++index) {
		instanceIndexData[index] = index;
	}
	instanceIndexBufferView_.BufferLocation = instanceIndexBuffer_.GetGpuVirtualAddress();
	instanceIndexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * numInstance_);
	instanceIndexBufferView_.StrideInBytes = sizeof(uint32_t);
}

void FractureInstance::WriteInstance(uint32_t index, const GeometryRange& range, uint32_t chunkId) {
	ResetState(transformData_[index]);

	instancingData_[index].world = Matrix4x4::MakeIdentity();
	instancingData_[index].worldInverseTranspose = Matrix4x4::MakeIdentity();
	instancingData_[index].vertexOffset = range.vertexOffset;
	instancingData_[index].indexOffset = range.indexOffset;
	instancingData_[index].indexCount = range.indexCount;
	instancingData_[index].chunkId = chunkId;

	// ExecuteIndirect用のDrawコマンド引数を設定
	argumentData_[index].drawArguments.IndexCountPerInstance = range.indexCount; // 破片のインデックス数
	argumentData_[index].drawArguments.InstanceCount = 1;                        // 1回につき1個描画
	argumentData_[index].drawArguments.StartIndexLocation = range.indexOffset;   // インデックスの開始位置
	argumentData_[index].drawArguments.BaseVertexLocation = static_cast<INT>(range.vertexOffset); // 頂点の開始位置
	// インスタンス単位の頂点バッファからindex番目(=このインスタンスの番号)を読ませる
	argumentData_[index].drawArguments.StartInstanceLocation = index;
}

void FractureInstance::ResetState(FractureChunkState& state) {
	state.transform.scale = { 1.0f, 1.0f, 1.0f };
	state.transform.rotate = { 0.0f, 0.0f, 0.0f };
	state.transform.translate = { 0.0f, 0.0f, 0.0f };
	state.velocity = { 0.0f, 0.0f, 0.0f };
	state.crackVelocity = { 0.0f, 0.0f, 0.0f };
	state.crackAngularVelocity = { 0.0f, 0.0f, 0.0f };
	state.crackRestOffset = { 0.0f, 0.0f, 0.0f };
	state.pivot = { 0.0f, 0.0f, 0.0f };
	state.bottomOffset = 0.0f;
	state.angularVelocity = { 0.0f, 0.0f, 0.0f };
	state.isGroundActive = false;
	state.isSleeping = false;
}

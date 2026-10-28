#include "pch.h"
#include "FractureStaticMesh.h"
using namespace GameEngine;

ID3D12GraphicsCommandList4* FractureStaticMesh::commandList_ = nullptr;

void FractureStaticMesh::Initialize(const PackedGeometryBuffer* geometryBuffer) {
	geometryBuffer_ = geometryBuffer;
	refBuffer_.Create();
}

void FractureStaticMesh::SetMaterial(uint32_t materialSrvIndex, uint32_t instanceMask) {
	if (geometryBuffer_ == nullptr) { return; }
	refBuffer_.SetBufferMaterial(0, materialSrvIndex);
	refBuffer_.SetInstanceMask(instanceMask);
}

void FractureStaticMesh::FlushIfDirty() {
	if (!isDirty_ || geometryBuffer_ == nullptr) {
		return;
	}
	isDirty_ = false;

	// 毎フレームGPUの完了を待っているため、前フレームで使っていたバッファはここで作り直してよい
	indexCount_ = static_cast<uint32_t>(pendingIndices_.size());
	if (indexCount_ == 0) {
		indexBuffer_.reset();
		blas_.reset();
		return;
	}

	indexBuffer_ = std::make_unique<IndexBuffer>();
	indexBuffer_->Create(pendingIndices_);

	// 頂点はPackedGeometryBuffer全体を共有し、インデックス側で静的なチャンクの三角形だけを参照する
	blas_ = std::make_unique<BLAS>();
	blas_->Create(commandList_, geometryBuffer_->GetVertexBufferView(), indexBuffer_->GetView(),
		geometryBuffer_->GetTotalVertices(), indexCount_, false);

	// インデックスは頂点バッファ全体を基準にしているので、オフセットは0
	refBuffer_.SetModelData(geometryBuffer_->GetVertexBufferSrvIndex(), indexBuffer_->GetSrvIndex(), 0, 0);

	pendingIndices_.clear();
}

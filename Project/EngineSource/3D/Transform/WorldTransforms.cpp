#include "pch.h"
#include "WorldTransforms.h"
#include "MyMath.h"
#include "CreateBufferResource.h"
#include "DescriptorHandle.h"
using namespace GameEngine;

WorldTransforms::~WorldTransforms() {

	transformDatas_.clear();
}
void WorldTransforms::Initialize(const uint32_t& kNumInstance, const Transform& transform) {

	TransformData transformData;
	for (uint32_t i = 0; i < kNumInstance; ++i) {
		transformData.transform = transform;
		transformData.worldMatrix = Matrix4x4::MakeAffineMatrix(transformData.transform.scale, transformData.transform.rotate, transformData.transform.translate);
		transformData.color = { 1.0f,1.0f,1.0f,1.0f };
		transformData.textureHandle = 0;
		transformDatas_.push_back(transformData);
	}

	// 頂点数を設定
	numInstance_ = kNumInstance;

	// リソース作成
	buffer_.Create(numInstance_);

	instancingData_ = buffer_.GetData();
	// 単位行列を書き込んでおく
	for (uint32_t index = 0; index < numInstance_; ++index) {
		instancingData_[index].World = Matrix4x4::MakeIdentity();
		instancingData_[index].color = { 1.0f,1.0f,1.0f,1.0f };
		instancingData_[index].textureHandle = 0;
	}
}

void WorldTransforms::UpdateTransformMatrix(const uint32_t& numInstance) {
	// 数によって更新を変える
	for (uint32_t i = 0; i < numInstance; ++i) {
		transformDatas_[i].worldMatrix = Math::MakeWorldMatrixFromEulerRotation(transformDatas_[i].transform.translate, transformDatas_[i].transform.rotate, transformDatas_[i].transform.scale);
	}
}

void WorldTransforms::SetWVPMatrix(const uint32_t& numInstance) {
	// 数によって更新を変える
	for (uint32_t i = 0; i < numInstance; ++i) {
		instancingData_[i].World = transformDatas_[i].worldMatrix;
		instancingData_[i].color = transformDatas_[i].color;
		instancingData_[i].textureHandle = transformDatas_[i].textureHandle;
	}
}

void WorldTransforms::SetWVPMatrix(const uint32_t& numInstance, const Matrix4x4& localMatrix) {

	// 数によって更新を変える
	for (uint32_t i = 0; i < numInstance; ++i) {
		instancingData_[i].World = localMatrix * transformDatas_[i].worldMatrix;
		instancingData_[i].color = transformDatas_[i].color;
		instancingData_[i].textureHandle = transformDatas_[i].textureHandle;
	}
}
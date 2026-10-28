#include "pch.h"
#include "IceDemo.h"

using namespace GameEngine;

IceDemo::IceDemo(std::string name, GameEngine::Model* model) : modelComponent_(model) {

	std::string subGroup = "";
	int index = 0;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>(name);
	debugParame_->Register("IceColor", iceMaterial_.materialData_->color);
	debugParame_->Register("IceRoughness", iceMaterial_.materialData_->roughness);
	debugParame_->Register("IceIor", iceMaterial_.materialData_->ior);
	debugParame_->RegisterWorld("", modelComponent_.worldTransform_);

	subGroup = "Surface";
	debugParame_->Register("IceChipScale", iceMaterial_.materialData_->chipScale, index++, subGroup);
	debugParame_->Register("IceChipStrength", iceMaterial_.materialData_->chipStrength, index++, subGroup);
	debugParame_->Register("IceEdgeWidth", iceMaterial_.materialData_->edgeWidth, index++, subGroup);
	debugParame_->Register("IceEdgeStrength", iceMaterial_.materialData_->edgeStrength, index++, subGroup);
	debugParame_->Register("IceMicroScale", iceMaterial_.materialData_->microScale, index++, subGroup);
	debugParame_->Register("IceMicroStrength", iceMaterial_.materialData_->microStrength, index++, subGroup);
	debugParame_->Register("IceDissolveThreshold", iceMaterial_.materialData_->dissolveThreshold, index++, subGroup);
	subGroup = "Bubble";
	index = 0;
	debugParame_->Register("bubbleScale", iceMaterial_.materialData_->bubbleScale, index++, subGroup);
	debugParame_->Register("bubbleMaxDepth", iceMaterial_.materialData_->bubbleMaxDepth, index++, subGroup);
	debugParame_->Register("bubbleDensity", iceMaterial_.materialData_->bubbleDensity, index++, subGroup);
	debugParame_->Register("bubbleJitter", iceMaterial_.materialData_->bubbleJitter, index++, subGroup);
	debugParame_->Register("bubbleHighlight", iceMaterial_.materialData_->bubbleHighlight, index++, subGroup);
	debugParame_->Apply();

	// マテリアルを設定
	modelComponent_.SetBufferMaterial(0, iceMaterial_.GetMaterialSrvIndex());
	modelComponent_.SetHitGroup(1);
	modelComponent_.SetInstanceMask(static_cast<uint32_t>(RayInstanceMask::kRayMaskIce));
}

void IceDemo::Initialize() {

}

void IceDemo::Update() {
	debugParame_->ApplyIfDirty();

	// 更新処理
	modelComponent_.Update();
}

void IceDemo::DebugUpdate() {
	Update();
}

void IceDemo::Draw() {
	modelComponent_.DrawRaytracing(renderQueue_);
}
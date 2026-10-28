#include "pch.h"
#include "BgIceRock.h"
using namespace GameEngine;

BgIceRock::BgIceRock(GameEngine::Model* model) : iceModelComponent_(model) {

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("BgIceRock");
	debugParame_->RegisterWorld("world", iceModelComponent_.worldTransform_);
	std::string subGroup = "IceMaterial";
	int index = 0;
	debugParame_->Register("Color", iceMaterial_.materialData_->color, index++, subGroup);
	debugParame_->Register("bubbleScale", iceMaterial_.materialData_->bubbleScale, index++, subGroup);
	debugParame_->Register("bubbleMaxDepth", iceMaterial_.materialData_->bubbleMaxDepth, index++, subGroup);
	debugParame_->Register("bubbleDensity", iceMaterial_.materialData_->bubbleDensity, index++, subGroup);
	debugParame_->Register("bubbleJitter", iceMaterial_.materialData_->bubbleJitter, index++, subGroup);
	debugParame_->Register("bubbleHighlight", iceMaterial_.materialData_->bubbleHighlight, index++, subGroup);
	debugParame_->Apply();

	// 参照するマテリアルを変更
	iceModelComponent_.SetBufferMaterial(0, iceMaterial_.GetMaterialSrvIndex());
	iceModelComponent_.SetHitGroup(1);

	Update();
}

void BgIceRock::Initialize() {
	iceModelComponent_.Update();
}

void BgIceRock::Update() {
	debugParame_->ApplyIfDirty();

	// 更新
	iceModelComponent_.Update();
}

void BgIceRock::Draw() {
	// 氷を描画
	iceModelComponent_.DrawRaytracing(renderQueue_);
}
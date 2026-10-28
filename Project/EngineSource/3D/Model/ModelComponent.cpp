#include "pch.h"
#include "ModelComponent.h"
#include "Model.h"
#include "RenderQueue.h"
using namespace GameEngine;

ModelComponent::ModelComponent(Model* model) {
	model_ = model;
	worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// モデルからマテリアル情報を取得
	auto& meshes = model_->GetMeshes();
	uint32_t meshNum = meshes.size();

	// 参照用
	refBuffers_.resize(meshNum);
	defaultMaterials_.resize(meshNum);
	for (size_t i = 0; i < meshNum; ++i) {
		Material* material = model_->GetMaterial(meshes[i]->GetMaterialName());
		auto data = material->GetMaterialData();
		// マテリアルを作成
		defaultMaterials_[i].Initialize(data->color, {1.0f,1.0f,1.0f}, data->shininess, data->enableLighting);
		materialData_ = defaultMaterials_[i].GetMaterialData();

		materialData_->textureHandle = data->textureHandle;
		materialData_->roughness = data->roughness;
		materialData_->metallic = data->metallic;

		refBuffers_[i].Create();
		refBuffers_[i].SetBufferMaterial(0, defaultMaterials_[i].GetMaterialSrvIndex());
	}
}

void ModelComponent::Update() {
	
	// 行列を更新
	worldTransform_.UpdateTransformMatrix();
}

void ModelComponent::Draw(RenderQueue* renderQueue, const Draw3dType& drawType, const std::string& passName) {

	switch (drawType)
	{
	case GameEngine::Draw3dType::Default:
		renderQueue->SubmitModel(model_, worldTransform_, materialData_->color.w, &defaultMaterials_[0].GetMaterialBuffer(), passName);
		break;

	case GameEngine::Draw3dType::DefaultAdd:
		renderQueue->SubmitAddModel(model_, worldTransform_, materialData_->color.w, &defaultMaterials_[0].GetMaterialBuffer(), passName);
		break;

	case GameEngine::Draw3dType::Animation:
		renderQueue->SubmitAnimation(model_, worldTransform_, materialData_->color.w, &defaultMaterials_[0].GetMaterialBuffer(), passName);
		break;

	case GameEngine::Draw3dType::ShadowMap:
		renderQueue->SubmitShadowMap(model_, worldTransform_);
		break;

	case GameEngine::Draw3dType::Instancing:
	case GameEngine::Draw3dType::InstancingAdd:
	case GameEngine::Draw3dType::Grid:
	case GameEngine::Draw3dType::Skybox:
	default:
		break;
	}
}

void ModelComponent::DrawRaytracing(RenderQueue* renderQueue) {
	// レイトレによる描画
	renderQueue->SubmitRaytracingModel(model_, worldTransform_, &refBuffers_);
}
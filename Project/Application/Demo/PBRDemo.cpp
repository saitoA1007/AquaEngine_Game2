#include "pch.h"
#include "PBRDemo.h"
#include "Model.h"
#include "ImGuiManager.h"
using namespace GameEngine;

PBRDemo::PBRDemo(Model* sphereModel, uint32_t metallicSteps, uint32_t roughnessSteps) {
	model_ = sphereModel;
	rows_ = (std::max)(metallicSteps, 1u);
	cols_ = (std::max)(roughnessSteps, 1u);

	spheres_.clear();
	spheres_.reserve(static_cast<size_t>(rows_) * cols_);

	for (uint32_t r = 0; r < rows_; ++r) {
		for (uint32_t c = 0; c < cols_; ++c) {
			spheres_.push_back(std::make_unique<ModelComponent>(model_));
		}
	}

	layoutDirty_ = true;
	ApplyLayout();
}

void PBRDemo::Initialize() {
	
}

void PBRDemo::Update() {
	if (layoutDirty_) {
		ApplyLayout();
	}

	for (uint32_t r = 0; r < rows_; ++r) {
		for (uint32_t c = 0; c < cols_; ++c) {
			auto& sphere = spheres_[r * cols_ + c];

			// マテリアルへ反映
			auto* data = sphere->materialData_;
			data->color = baseColor_;
			data->metallic = GetMetallic(r);
			data->roughness = GetRoughness(c);

			sphere->Update();
		}
	}

#ifdef USE_IMGUI
	ImGui::Begin("PBR Demo");

	ImGui::ColorEdit4("BaseColor", &baseColor_.x);

	if (ImGui::DragFloat("Spacing", &spacing_, 0.05f, 0.5f, 20.0f)) { layoutDirty_ = true; }
	if (ImGui::DragFloat("Sphere Scale", &sphereScale_, 0.05f, 0.1f, 10.0f)) { layoutDirty_ = true; }
	if (ImGui::DragFloat3("Origin", &origin_.x, 0.1f)) { layoutDirty_ = true; }

	ImGui::DragFloatRange2("Roughness Range", &minRoughness_, &maxRoughness_, 0.01f, 0.01f, 1.0f);

	ImGui::End();
#endif
}

void PBRDemo::Draw() {
	// ラスタライズ
	//for (auto& sphere : spheres_) {
	//	sphere->Draw(renderQueue_);
	//}
	// レイトレ
	for (auto& sphere : spheres_) {
		sphere->DrawRaytracing(renderQueue_);
	}
}

void PBRDemo::DebugUpdate() {

}

void PBRDemo::ApplyLayout() {
	// グリッドの中心がoriginに来るようにオフセット
	const float offsetX = (static_cast<float>(cols_) - 1.0f) * 0.5f;
	const float offsetY = (static_cast<float>(rows_) - 1.0f) * 0.5f;

	for (uint32_t r = 0; r < rows_; ++r) {
		for (uint32_t c = 0; c < cols_; ++c) {
			Vector3 pos = {
				origin_.x + (static_cast<float>(c) - offsetX) * spacing_,
				origin_.y + (static_cast<float>(r) - offsetY) * spacing_,
				origin_.z
			};
			spheres_[r * cols_ + c]->worldTransform_.Initialize({
				{ sphereScale_, sphereScale_, sphereScale_ },
				{ 0.0f, 0.0f, 0.0f },
				pos
				});
		}
	}
	layoutDirty_ = false;
}

float PBRDemo::GetMetallic(uint32_t row) const {
	if (rows_ <= 1) { return 0.0f; }
	return static_cast<float>(row) / static_cast<float>(rows_ - 1);
}

float PBRDemo::GetRoughness(uint32_t col) const {
	if (cols_ <= 1) { return minRoughness_; }
	const float t = static_cast<float>(col) / static_cast<float>(cols_ - 1);
	return minRoughness_ + (maxRoughness_ - minRoughness_) * t;
}
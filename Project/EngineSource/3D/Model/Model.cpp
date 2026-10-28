#include "pch.h"
#include "Model.h"
#include <cassert>
using namespace GameEngine;

void  Model::SetDefaultColor(const Vector4& color, const std::string& materialName) {

	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetColor(color);
}

void Model::SetDefaultSpecularColor(const Vector3& specularColor, const std::string& materialName) {

	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetSpecularColor(specularColor);
}

void Model::SetDefaultShininess(const float& shininess, const std::string& materialName) {

	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetShininess(shininess);
}

void  Model::SetDefaultIsEnableLight(const bool& isEnableLight, const std::string& materialName) {

	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetEnableLighting(isEnableLight);
}

void Model::SetDefaultIsEnableShadow(const bool& isEnableShadow, const std::string& materialName) {
	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetEnableShadow(isEnableShadow);
}

void  Model::SetDefaultUVMatrix(const Matrix4x4& uvMatrix, const std::string& materialName) {
	
	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetUVMatrix(uvMatrix);
}

void Model::SetDefaultUVMatrix(const Transform& uvTransform, const std::string& materialName) {

	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetUVTransform(uvTransform);
}

void Model::SetDefaultTextureHandle(const uint32_t& handle, const std::string& materialName) {
	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetTextureHandle(handle);
}

void  Model::SetDefaultMetallic(const float& metallic, const std::string& materialName) {
	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetMetallic(metallic);
}

void Model::SetDefaultIOR(const float& ior, const std::string& materialName) {
	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetIOR(ior);
}

void Model::SetRoughness(const float& roughness, const std::string& materialName) {
	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetRoughness(roughness);
}

void Model::SetDefaultNormalTexture(const uint32_t& texture, const std::string& materialName) {
	auto it = materialName == "default" ? materials_.begin() : materials_.find(materialName);

	assert(it != materials_.end() && "Material not found");
	Material* material = it->second.get();
	material->SetNormalTexture(texture);
}

Material* Model::GetMaterial(const std::string& name) const {
	auto it = materials_.find(name);
	if (it != materials_.end()) {
		return it->second.get();
	}
	
	assert(it != materials_.end() && "Material not found");
	return nullptr;
}


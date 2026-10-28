#include "pch.h"
#include "ModelManager.h"
#include <filesystem>
#include <iostream>
#include "ParallelFor.h"
#include "LogManager.h"

using namespace GameEngine;

ModelManager::~ModelManager() {
	if (parseFuture_.valid()) {
		parseFuture_.wait();
	}
	models_.clear();
}

void ModelManager::Initialize(ID3D12GraphicsCommandList4* cmdList, TextureManager* textureManager, SrvManager* srvManager) {
	// モデル生成システムの初期化
	loader_.Initialize(cmdList, textureManager, srvManager);
}

void ModelManager::ApplyTexutre(TextureManager* textureManager) {

	// モデルに存在するテクスチャを実際に適応
	for (auto& model : models_) {
		ModelEntryData& data = model.second;
		auto& materials = data.model->GetMaterials();
		for (auto& [key, material] : materials) {
			uint32_t textureHandle = textureManager->GetHandleByName(material->GetTextureName());
			material->SetTextureHandle(textureHandle);
			material->SetDefaultTexture(textureHandle);
		}
	}
}

void ModelManager::BeginLoadAllModel() {
	// ロードするファイル一覧を作る
	loadFiles_ = EnumerateModelFiles();
	parsedModels_.clear();
	parsedModels_.resize(loadFiles_.size());

	// 背景でパース開始
	parseFuture_ = std::async(std::launch::async, [this] {
		ParallelFor(loadFiles_.size(), [this](size_t i) {
			try {
				parsedModels_[i] = loader_.ParseModel(loadFiles_[i].fileName, loadFiles_[i].folderPath);
			}
			catch (...) {
				parsedModels_[i] = nullptr;
			}
			});
		});
}

void ModelManager::FinishLoadAllModel() {
	// パース完了を待つ
	if (parseFuture_.valid()) { parseFuture_.get(); }

	// GPUリソース作成と登録
	for (size_t i = 0; i < loadFiles_.size(); ++i) {
		if (!parsedModels_[i]) { continue; }
		RegisterModel(loadFiles_[i].fileName,
			loader_.BuildModel(loadFiles_[i].fileName, std::move(parsedModels_[i])));
	}
	parsedModels_.clear();
	loadFiles_.clear();
}

void ModelManager::RegisterModel(const std::string& modelFile, const std::string& objFileName) {

	// 同名のモデルが登録されている場合は早期リターン
	auto getName = nameToHandles_.find(objFileName);
	if (getName != nameToHandles_.end()) {
		return;
	}

	// 新しいハンドルを取得
	uint32_t handle = nextHandle_++;

	// 登録データするを作成
	ModelEntryData entryData;
	entryData.name = objFileName;
	entryData.model = loader_.CreateModel(objFileName, modelFile);

	// 登録する
	models_[handle] = std::move(entryData);
	nameToHandles_[objFileName] = handle;
}

void ModelManager::RegisterModel(const std::string& modelName, std::unique_ptr<Model> model) {
	// 同名のモデルが登録されている場合は早期リターン
	auto getName = nameToHandles_.find(modelName);
	if (getName != nameToHandles_.end()) {
		return;
	}

	// 新しいハンドルを取得
	uint32_t handle = nextHandle_++;

	// 登録データするを作成
	ModelEntryData entryData;
	entryData.name = modelName;
	entryData.model = std::move(model);

	// 登録する
	models_[handle] = std::move(entryData);
	nameToHandles_[modelName] = handle;
}

void  ModelManager::RegisterGridPlaneModel(const std::string& modelName, const Vector2& size) {
	// 同名のモデルが登録されている場合は早期リターン
	auto getName = nameToHandles_.find(modelName);
	if (getName != nameToHandles_.end()) {
		return;
	}

	// 新しいハンドルを取得
	uint32_t handle = nextHandle_++;

	// 登録データするを作成
	ModelEntryData entryData;
	entryData.name = modelName;
	entryData.model = loader_.CreateGridPlane(size);

	// 登録する
	models_[handle] = std::move(entryData);
	nameToHandles_[modelName] = handle;
}

void ModelManager::RegisterRingModel(const std::string& modelName, uint32_t ringDivide, float outerRadius, float innerRadius) {
	// 同名のモデルが登録されている場合は早期リターン
	auto getName = nameToHandles_.find(modelName);
	if (getName != nameToHandles_.end()) {
		return;
	}

	// 新しいハンドルを取得
	uint32_t handle = nextHandle_++;

	// 登録データするを作成
	ModelEntryData entryData;
	entryData.name = modelName;
	entryData.model = loader_.CreateRing(ringDivide, outerRadius, innerRadius);

	// 登録する
	models_[handle] = std::move(entryData);
	nameToHandles_[modelName] = handle;
}

void ModelManager::RegisterCylinderModel(const std::string& modelName, uint32_t cylinderDivide, float topRadius, float bottomRadius, float height) {
	// 同名のモデルが登録されている場合は早期リターン
	auto getName = nameToHandles_.find(modelName);
	if (getName != nameToHandles_.end()) {
		return;
	}

	// 新しいハンドルを取得
	uint32_t handle = nextHandle_++;

	// 登録データするを作成
	ModelEntryData entryData;
	entryData.name = modelName;
	entryData.model = loader_.CreateCylinder(cylinderDivide, topRadius, bottomRadius, height);

	// 登録する
	models_[handle] = std::move(entryData);
	nameToHandles_[modelName] = handle;
}

void ModelManager::UnregisterModel(uint32_t handle) {
	auto getModel = models_.find(handle);
	if (getModel == models_.end()) {
		return;
	}

	// 名前からマップを削除
	const std::string& name = getModel->second.name;
	nameToHandles_.erase(name);

	// 登録したモデル本体を削除
	models_.erase(getModel);
}

uint32_t ModelManager::GetHandleByName(const std::string& name) const {
	auto getHandle = nameToHandles_.find(name);
	if (getHandle == nameToHandles_.end()) {
		return 0;
	}
	return getHandle->second;
}

std::string ModelManager::GetNameByHandle(uint32_t handle) const {
	auto getName = models_.find(handle);
	if (getName == models_.end()) {
		return "";
	}
	return getName->second.name;
}

[[nodiscard]]
Model* ModelManager::GetHandleByModel(uint32_t handle) const {
	auto getModel = models_.find(handle);
	if (getModel == models_.end()) {
		return nullptr;
	}

	if (getModel->second.model) {
		return getModel->second.model.get();
	}

	return nullptr;
}

[[nodiscard]]
Model* ModelManager::GetNameByModel(const std::string& name) const {

	auto getHandle = nameToHandles_.find(name);
	if (getHandle == nameToHandles_.end()) {
		return nullptr;
	}

	auto getModel = models_.find(getHandle->second);
	if (getModel == models_.end()) {
		return nullptr;
	}

	if (getModel->second.model) {
		return getModel->second.model.get();
	}

	return nullptr;
}

std::vector<ModelManager::ModelFile> ModelManager::EnumerateModelFiles() const {
	const std::string kDirectoryPath = "Resources/Models";
	std::vector<ModelFile> files;
	if (!std::filesystem::exists(kDirectoryPath)) { return files; }

	const std::vector<std::string> allowedExtensions = { ".obj", ".gltf" };
	std::unordered_set<std::string> seenNames;

	try {
		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(kDirectoryPath)) {
			if (!dirEntry.is_regular_file()) { continue; }

			const std::string extension = dirEntry.path().extension().string();
			if (std::find(allowedExtensions.begin(), allowedExtensions.end(), extension) == allowedExtensions.end()) {
				continue;
			}

			std::string fileName = dirEntry.path().filename().string();
			if (nameToHandles_.contains(fileName) || !seenNames.insert(fileName).second) {
				continue;
			}

			auto relativePath = std::filesystem::relative(dirEntry.path(), kDirectoryPath);
			std::string folderPath = relativePath.parent_path().string();
			std::replace(folderPath.begin(), folderPath.end(), '\\', '/');

			files.push_back({ folderPath, fileName });
		}
	}
	catch (std::filesystem::filesystem_error& e) {
		std::cerr << "Filesystem error while loading models: " << e.what() << std::endl;
	}
	return files;
}
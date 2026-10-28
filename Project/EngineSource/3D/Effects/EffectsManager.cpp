#include "pch.h"
#include "EffectsManager.h"
#include "JsonSerializer.h"
#include "LogManager.h"

using namespace GameEngine;

EffectsManager::EffectsManager(TextureManager* textureManager, ModelManager* modelManager) {
	textureManager_ = textureManager;
	modelManager_ = modelManager;
}

void EffectsManager::Initialize() {
	LoadAll();
}

void EffectsManager::Update() {
	for (auto& [name, list] : instances_) {
		for (auto& instance : list) {
			instance->Update();
		}
	}
	for (auto& instance : retiredInstances_) {
		instance->Update();
	}
}

void EffectsManager::Draw() {
	for (auto& [name, list] : instances_) {
		for (auto& instance : list) {
			instance->Draw();
		}
	}
	for (auto& instance : retiredInstances_) {
		instance->Draw();
	}
}

void EffectsManager::LoadAll() {
	if (!JsonSerializer::DirectoryExists(EffectAsset::kDirectoryPath)) {
		return;
	}

	JsonSerializer::LoadDirectory(EffectAsset::kDirectoryPath,
		[this](const std::string& name, const nlohmann::json& root) {
			try {
				RegisterEffect(EffectAsset::FromJson(name, root));
			} catch (const std::exception& e) {
				Log("EffectsManager: Failed to load " + name + " : " + e.what());
			}
		});
}

void EffectsManager::RegisterEffect(const EffectAsset& asset) {
	assets_[asset.name] = asset;

	// 古い定義で作られたインスタンスは再利用しないように移動する
	auto it = instances_.find(asset.name);
	if (it != instances_.end()) {
		for (auto& instance : it->second) {
			retiredInstances_.push_back(std::move(instance));
		}
		instances_.erase(it);
	}
}

std::unique_ptr<EffectObject> EffectsManager::GetEffect(const std::string& name) const {
	const EffectAsset* asset = GetAsset(name);
	if (asset == nullptr) {
		Log("EffectsManager: effect \"" + name + "\" not found");
		return nullptr;
	}
	return std::make_unique<EffectObject>(*asset, textureManager_, modelManager_);
}

const EffectAsset* EffectsManager::GetAsset(const std::string& name) const {
	auto it = assets_.find(name);
	if (it == assets_.end()) {
		return nullptr;
	}
	return &it->second;
}

EffectObject* EffectsManager::Play(const std::string& name, const Vector3& pos) {
	auto& list = instances_[name];

	// 再生が終わっているインスタンスを再利用する
	for (auto& instance : list) {
		if (!instance->IsPlaying()) {
			instance->Play(pos);
			return instance.get();
		}
	}

	// 無ければ新しく生成する
	std::unique_ptr<EffectObject> newInstance = GetEffect(name);
	if (newInstance == nullptr) {
		return nullptr;
	}
	list.push_back(std::move(newInstance));
	EffectObject* instance = list.back().get();
	instance->Play(pos);
	return instance;
}

void EffectsManager::StopAll() {
	for (auto& [name, list] : instances_) {
		for (auto& instance : list) {
			instance->StopImmediate();
		}
	}
	for (auto& instance : retiredInstances_) {
		instance->StopImmediate();
	}
}

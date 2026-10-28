#include "pch.h"
#include "AnimationManager.h"

#include "ModelLoader.h"
#include "JsonSerializer.h"
using namespace GameEngine;

AnimationManager::~AnimationManager() {
	animations_.clear();
}

void AnimationManager::RegisterAnimation(const std::string& registerName, const std::string& objFilename, const std::string& directory) {
	
	// 同名のアニメーションデータが登録されている場合は早期リターン
	auto getName = animations_.find(registerName);
	if (getName != animations_.end()) {
		return;
	}

	// 指定したモデルに存在するアニメーションデータをを全て取得する
	animations_[registerName] = ModelLoader::LoadAnimationsFile(objFilename, directory);
}

void AnimationManager::UnregisterAnimation(const std::string& name) {
	auto getAnimation = animations_.find(name);
	if (getAnimation == animations_.end()) {
		return;
	}

	// 登録したアニメーションデータを削除
	animations_.erase(getAnimation);
}

[[nodiscard]]
const AnimationData& AnimationManager::GetNameByAnimation(const std::string& name) const {
	 auto getAnimations = animations_.find(name);

	 // 見つからなかった場合のエラーハンドリング
	 if (getAnimations == animations_.end()) {
		 assert(0 && "Animation File Not Found");
	 }

	 const auto& animationMap = getAnimations->second;

	 // アニメーションデータが空でないか確認
	 if (animationMap.empty()) {
		 assert(0 && "Animation Data is Empty");
	 }

	 return animationMap.begin()->second;
}

[[nodiscard]]
const std::map<std::string, AnimationData>& AnimationManager::GetNameByAnimations(const std::string& name) const {
	auto getAnimations = animations_.find(name);
	if (getAnimations == animations_.end()) {
		assert(0);
	}

	// アニメーションデータ達を返す
	return getAnimations->second;
}

void AnimationManager::LoadAllAnimation() {

	// 登録ファイルが無い場合は何も読み込まない
	if (!JsonSerializer::FileExists(kRegisterFilePath)) {
		return;
	}

	// 登録ファイルを読み込む
	const nlohmann::json root = JsonSerializer::LoadFromFile(kRegisterFilePath);
	if (!root.is_object()) {
		return;
	}

	// <登録名> : { "file": ファイル名, "directory": ディレクトリ } の形式で登録する
	for (const auto& [registerName, entry] : root.items()) {
		if (!entry.contains("file") || !entry.contains("directory")) {
			continue;
		}
		RegisterAnimation(registerName, entry["file"].get<std::string>(), entry["directory"].get<std::string>());
	}
}
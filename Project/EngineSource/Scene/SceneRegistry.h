#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <type_traits>
#include "IScene.h"

namespace GameEngine {

	/// <summary>
	/// シーンを登録・生成する
	/// </summary>
	class SceneRegistry {
	public:
		SceneRegistry() = default;
		~SceneRegistry() = default;

		/// <summary>
		/// シーンを登録する
		/// </summary>
		/// <typeparam name="T">ISceneを継承したクラス</typeparam>
		/// <param name="sceneName"></param>
		template<typename T>
		void RegisterScene(const std::string& sceneName) {
			static_assert(std::is_base_of<IScene, T>::value, "T must derive from IScene");
			creators_[sceneName] = []() { return std::make_unique<T>(); };
			sceneNames_.push_back(sceneName);
		}

		/// <summary>
		/// シーンを生成する
		/// </summary>
		/// <param name="sceneName">シーン名</param>
		/// <returns></returns>
		std::unique_ptr<IScene> CreateScene(const std::string& sceneName) {
			auto it = creators_.find(sceneName);
			if (it != creators_.end()) {
				return it->second();
			}
			return nullptr;
		}

		/// <summary>
		/// シーンが登録されているか確認する
		/// </summary>
		/// <param name="sceneName">シーン名</param>
		/// <returns></returns>
		bool HasScene(const std::string& sceneName) const {
			return creators_.find(sceneName) != creators_.end();
		}

		/// <summary>
		/// 登録されているシーン名のリストを取得
		/// </summary>
		/// <returns></returns>
		const std::vector<std::string>& GetSceneNames() const {
			return sceneNames_;
		}

		/// <summary>
		/// デフォルトシーンを設定する
		/// </summary>
		/// <param name="sceneName"></param>
		void SetDefaultScene(const std::string& sceneName) {
			defaultSceneName_ = sceneName;
		}

		/// <summary>
		/// デフォルトシーン名を取得する
		/// </summary>
		/// <returns></returns>
		const std::string& GetDefaultScene() const {
			return defaultSceneName_;
		}

	private:
		SceneRegistry(const SceneRegistry&) = delete;
		SceneRegistry& operator=(const SceneRegistry&) = delete;

		// シーン生成マップ
		std::unordered_map<std::string, std::function<std::unique_ptr<IScene>()>> creators_;
		// シーン名リスト
		std::vector<std::string> sceneNames_;
		// デフォルトシーン名
		std::string defaultSceneName_;
	};
}


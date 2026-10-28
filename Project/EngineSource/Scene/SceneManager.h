#pragma once
#include <string>
#include "SceneRegistry.h"
#include "SceneTransition.h"
#include "IScene.h"
#include "Camera.h"

namespace GameEngine {

	// 前方宣言
	class GameParamEditor;
	class GameObjectManager;
	class StaticGameObjectManager;

	/// <summary>
	/// シーンの管理
	/// </summary>
	class SceneManager {
	public:
		SceneManager() = default;
		~SceneManager();

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="context"></param>
		void Initialize(SceneRegistry* sceneRegistry, GameParamEditor* gameParamEditor, GameObjectManager* gameObjectManager, StaticGameObjectManager* staticObjectManager);

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update();

		/// <summary>
		/// デバック中でも動かせるようにするシーンの更新処理
		/// </summary>
		void DebugSceneUpdate();

		/// <summary>
		/// デバック時の更新処理
		/// </summary>
		void DebugUpdate();

		/// <summary>
		/// 描画処理
		/// </summary>
		void Draw();

		/// <summary>
		/// シーンの切り替え処理
		/// </summary>
		/// <param name="sceneName">切り替え先のシーン名</param>
		void ChangeScene(const std::string& sceneName);

		/// <summary>
		/// 現在のシーンをリセットする
		/// </summary>
		void ResetCurrentScene();

		/// <summary>
		/// 現在のシーン名を取得する
		/// </summary>
		/// <returns></returns>
		const std::string& GetCurrentSceneName() const { return currentSceneName_; }

	private: // エンジン機能

		// シーン機能
		SceneRegistry* sceneRegistry_ = nullptr;

		// パラメータ機能
		GameParamEditor* gameParamEditor_ = nullptr;

		// ゲームオブジェクト機能
		GameObjectManager* gameObjectManager_ = nullptr;

		// 静的ゲームオブジェクト機能
		StaticGameObjectManager* staticObjectManager_ = nullptr;

	private: // シーン機能

		// 現在のシーン
		std::unique_ptr<IScene> currentScene_;

		// シーンの切り替え処理をしているか判断する
		bool isChangeScene_ = false;

		// 現在のシーン名
		std::string currentSceneName_;

		// シーン遷移
		std::unique_ptr<SceneTransition> sceneTransition_;
	};
}

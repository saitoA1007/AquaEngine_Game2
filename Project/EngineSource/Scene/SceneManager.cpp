#include "pch.h"
#include "SceneManager.h"
#include "GameParamEditor.h"
#include "StaticGameObjectManager.h"
using namespace GameEngine;

SceneManager::~SceneManager() {
	currentScene_.reset();
	currentScene_.release();
}

void SceneManager::Initialize(SceneRegistry* sceneRegistry, GameParamEditor* gameParamEditor, GameObjectManager* gameObjectManager, StaticGameObjectManager* staticObjectManager) {
	gameParamEditor_ = gameParamEditor;
	gameObjectManager_ = gameObjectManager;
	staticObjectManager_ = staticObjectManager;

	// シーンの生成機能を初期化
	sceneRegistry_ = sceneRegistry;

	//　シーン遷移システムを初期化
	sceneTransition_ = std::make_unique<SceneTransition>();
	sceneTransition_->Initialize();
	
	/// デフォルトシーンで初期化
	// シーン名を保存
	currentSceneName_ = sceneRegistry_->GetDefaultScene();
	// 現在のシーンを設定
	gameParamEditor_->SetActiveScene(currentSceneName_);
	// 前の要素を削除
	currentScene_.reset();
	gameObjectManager_->ClearAll();
	// 新しいシーンを作成
	currentScene_ = sceneRegistry_->CreateScene(currentSceneName_);
	// 新しく作ったシーンを初期化
	currentScene_->Initialize();
}

void SceneManager::Update() {

	// シーン遷移の更新処理
	sceneTransition_->Update();

	// シーンの切り替え
	if (sceneTransition_->IsMidTransition() && isChangeScene_) {
		isChangeScene_ = false;
		ChangeScene(currentScene_->NextSceneName());
	}

	// シーン遷移の開始
	if (currentScene_->IsFinished() && !isChangeScene_ && !sceneTransition_->IsActive()) {
		if (isChangeScene_ || sceneTransition_->IsActive()) {
			return;
		}
		isChangeScene_ = true;
		// トランジション開始
		sceneTransition_->Start(currentScene_->GetTransitionEffect());
	}

	// 遷移演出中でなければシーンを更新
	if (!sceneTransition_->IsActive()) {
		// 現在シーンの更新処理
		currentScene_->Update();
	}
}

void SceneManager::DebugUpdate() {

}

void SceneManager::DebugSceneUpdate() {
	//　停止中でも適応される処理
	currentScene_->DebugUpdate();
}

void SceneManager::Draw() {

	// 現在シーンの描画処理
	currentScene_->Draw();

	// シーン遷移演出を描画
	//sceneTransition_->Draw();
}

void SceneManager::ChangeScene(const std::string& sceneName) {

	// シーン名を保存
	currentSceneName_ = sceneName;

	// 現在のシーンを設定
	gameParamEditor_->SetActiveScene(currentSceneName_);

	// 前の要素を削除
	staticObjectManager_->Clear();
	gameObjectManager_->ClearAll();
	currentScene_.reset();

	// シーンの静的オブジェクトをロードする
	staticObjectManager_->LoadSceneObject(sceneName, false);

	// 新しいシーンを作成
	currentScene_ = sceneRegistry_->CreateScene(sceneName);

	if (currentScene_) {
		// 新しく作ったシーンを初期化
		currentScene_->Initialize();
		// 1回だけ更新処理を挟む
		currentScene_->Update();
	} else {
		// 新しいシーンのインスタンスを作れなかった場合
		assert(0 && "Scene not found");
	}
}

void SceneManager::ResetCurrentScene() {
	// 現在のシーンを再初期化する
	//ChangeScene(currentSceneName_);
	currentScene_->Initialize();
}
#include "pch.h"
#include "SceneSubsystem.h"
#include "ResourceSubsystem.h"
#include "GraphicsSubsystem.h"
#include "Collider.h"
#include "IGameObject.h"
using namespace GameEngine;

SceneSubsystem::SceneSubsystem() = default;
SceneSubsystem::~SceneSubsystem() = default;

void SceneSubsystem::Initialize() {
    auto* renderQueue = context_.graphics->GetRenderQueue();
    auto* modelManager = context_.resource->GetModelManager();

    // 当たり判定
    collisionManager_ = std::make_unique<CollisionManager>();
    Collider::StaticInitialize(collisionManager_.get());

    // ゲームオブジェクト管理
    gameObjectManager_ = std::make_unique<GameObjectManager>();
    // ゲームオブジェクト基底クラスの静的初期化
    IGameObject::StaticInitialize(renderQueue);

    // 配置用ゲームオブジェクト管理
    staticObjectManager_ = std::make_unique<StaticGameObjectManager>();
    staticObjectManager_->Initialize(gameObjectManager_.get(), modelManager);

    // シーン生成システム
    sceneRegistry_ = std::make_unique<SceneRegistry>();

    // シーンマネージャ
    sceneManager_ = std::make_unique<SceneManager>();

    // シーン切り替えリクエスト
    sceneChangeRequest_ = std::make_unique<SceneChangeRequest>();
}

void SceneSubsystem::SceneInitialize() {
    sceneManager_->Initialize(sceneRegistry_.get(), context_.resource->GetGameParamEditor(), gameObjectManager_.get(), staticObjectManager_.get());
    // 現在のシーンのオブジェクトを読み込む
    staticObjectManager_->LoadSceneObject(context_.resource->GetGameParamEditor()->GetActiveScene());
}

void SceneSubsystem::UpdateGameplay() {
    gameObjectManager_->UpdateAll();
    sceneManager_->Update();
    collisionManager_->CheckAllCollisions();
}

void SceneSubsystem::UpdateDebug() {
    sceneManager_->DebugSceneUpdate();
}

void SceneSubsystem::Draw() {
    sceneManager_->Draw();
    gameObjectManager_->DrawAll();
}

void SceneSubsystem::ResetCurrentScene() {
    // シーンの初期化
    sceneManager_->ResetCurrentScene();
    // ゲームオブジェクトを初期化
    gameObjectManager_->InitializeAll();
}

void SceneSubsystem::ChangeScene(const std::string& sceneName) {
    sceneManager_->ChangeScene(sceneName);
}

std::string SceneSubsystem::GetCurrentSceneName() const {
    return sceneManager_->GetCurrentSceneName();
}

void SceneSubsystem::Finalize() {
    sceneManager_.reset();
}
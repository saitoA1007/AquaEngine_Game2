#include "pch.h"
#include "GameScene.h"
#include "ImguiManager.h"
using namespace GameEngine;

#include "PostProcess/PostEffectData.h"
#include <cassert>

GameScene::~GameScene() {
}

GameScene::GameScene() {
	// 入力コマンド設定s
	InputRegisterCommand();

	// アセットを取得する
	LoadAssets();

	// 調整値を登録する
	RegisterDebugParameters();

	// 背景・カメラ・ライトを設定する
	SetUpEnvironment();

	// ゲームオブジェクトを生成する
	CreateGameObjects(cylinderModel_, sphereModel_);
}

void GameScene::Initialize() {


}

void GameScene::Update() {
	debugParam_->ApplyIfDirty();

	// ライトの調整値を反映する
	UpdateLight();

	// シーン更新はオブジェクト更新の後に走るため、更新後のプレイヤー位置を敵が使える
	StepEnemies();

	// 押し戻し後のプレイヤー位置を追従する
	gameCamera_->Update(player_->GetPosition());
}

void GameScene::Draw() {

}

void GameScene::LoadAssets() {
	// モデルを取得する
	cylinderModel_ = modelManager_->GetNameByModel("Cylinder");
	sphereModel_ = modelManager_->GetNameByModel("sphere.obj");

	// 背景のテクスチャを取得する
	skyboxTextureHandle_ = textureManager_->GetHandleByName("qwantani_moon_noon_puresky_1k.dds");

	// 氷ゲージの矩形に使う白いテクスチャを取得する
	whiteTextureHandle_ = textureManager_->GetHandleByName("white2x2.png");

	// 取得できなかったモデルがないか確認する
	assert(cylinderModel_ != nullptr);
	assert(sphereModel_ != nullptr);
}

void GameScene::RegisterDebugParameters() {
	// サブグループ名。ここを書き換えれば登録の全てに反映される
	const std::string kLightGroup = "Light";
	const std::string kPlayerStatsGroup = "PlayerStats";

	debugParam_ = std::make_unique<DebugParameter>("GameScene");

	// ライト
	debugParam_->Register("lightIntensity", lightIntensity_, 0, kLightGroup);
	debugParam_->Register("lightDirection", lightDirection_, 1, kLightGroup);

	// プレイヤーの強化値
	debugParam_->Register("moveSpeed", playerStats_.moveSpeed, 0, kPlayerStatsGroup);

	debugParam_->Apply();
}

void GameScene::SetUpEnvironment() {
	// 背景を設定
	renderQueue_->SetSkyboxTexture(skyboxTextureHandle_);

	// ゲーム専用カメラを生成し、描画に使用するカメラとして設定
	gameCamera_ = std::make_unique<GameCamera>(1280, 720);
	renderQueue_->SetCamera(gameCamera_->GetCamera());

	// ライトを設定
	UpdateLight();
}

void GameScene::CreateGameObjects(Model* cylinderModel, Model* sphereModel) {
	// フィールド
	field_ = gameObjectManager_->AddObject<Field>(cylinderModel);

	// プレイヤー
	player_ = gameObjectManager_->AddObject<Player>(sphereModel, sphereModel, inputCommand_, field_, &playerStats_);

	// 敵（敵一覧とモデルの読み込みに失敗すると例外を投げる）
	enemyManager_ = gameObjectManager_->AddObject<EnemyManager>(modelManager_, field_->GetRadius(), gameCamera_->GetCamera(), whiteTextureHandle_);
}

void GameScene::StepEnemies() {
	// 敵が見るプレイヤーの情報をまとめる
	EnemyInput input;
	input.playerPosition = player_->GetPosition();
	input.playerRadius = player_->GetRadius();
	input.iceAreaRadius = player_->GetIceAreaRadius();
	input.fieldRadius = field_->GetRadius();

	// 敵を更新し、結果を受け取る
	EnemyOutput output = enemyManager_->UpdateEnemies(input);

	// 結果をプレイヤーへ反映する
	player_->TakeDamage(output.damage);
	player_->ApplyPushback(output.pushback);
}

void GameScene::UpdateLight() {
	LightManager* light = renderQueue_->GetLightManager();
	light->directionalLight_->SetLightIntensity(lightIntensity_);
	light->SetDirectionalDirction(lightDirection_);
}

void GameScene::InputRegisterCommand() {

}

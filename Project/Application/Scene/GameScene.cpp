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

	// シーン更新はオブジェクト更新の後に走るため、更新後のプレイヤー位置を使える
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
	player_ = gameObjectManager_->AddObject<Player>(sphereModel, inputCommand_, field_, &playerStats_);
}

void GameScene::UpdateLight() {
	LightManager* light = renderQueue_->GetLightManager();
	light->directionalLight_->SetLightIntensity(lightIntensity_);
	light->SetDirectionalDirction(lightDirection_);
}

void GameScene::InputRegisterCommand() {

	// 決定ボタン
	inputCommand_->RegisterCommand("PauseAction", { {InputState::KeyTrigger, DIK_M},{InputState::PadTrigger, XINPUT_GAMEPAD_START} });
	inputCommand_->RegisterCommand("Decision", { {InputState::KeyTrigger, DIK_SPACE},{InputState::PadTrigger, XINPUT_GAMEPAD_A} });
	inputCommand_->RegisterCommand("SelectUp", { {InputState::KeyTrigger, DIK_W },{InputState::PadLeftStick,0,{0.0f,1.0f},0.2f}, { InputState::PadTrigger, XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("SelectDown", { {InputState::KeyTrigger, DIK_S },{InputState::PadLeftStick,0,{0.0f,-1.0f},0.2f}, {InputState::PadTrigger, XINPUT_GAMEPAD_DPAD_DOWN} });

	// 移動の入力コマンドを登録する
	inputCommand_->RegisterCommand("MoveUp", { {InputState::KeyPush, DIK_W },{InputState::PadLeftStick,0,{0.0f,1.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveDown", { {InputState::KeyPush, DIK_S },{InputState::PadLeftStick,0,{0.0f,-1.0f},0.2f}, {InputState::PadPush, XINPUT_GAMEPAD_DPAD_DOWN} });
	inputCommand_->RegisterCommand("MoveLeft", { {InputState::KeyPush, DIK_A },{InputState::PadLeftStick,0,{-1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { {InputState::KeyPush, DIK_D },{InputState::PadLeftStick,0,{1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_RIGHT } });
	// ジャンプコマンドを登録する
	inputCommand_->RegisterCommand("Jump", { {InputState::KeyTrigger, DIK_SPACE},{InputState::PadTrigger, XINPUT_GAMEPAD_A} });

	// カメラ操作のコマンドを登録する
	inputCommand_->RegisterCommand("CameraMoveLeft", { { InputState::KeyPush, DIK_LEFT },{InputState::PadRightStick,0,{-1.0f,0.0f},0.2f} });
	inputCommand_->RegisterCommand("CameraMoveRight", { { InputState::KeyPush, DIK_RIGHT },{InputState::PadRightStick,0,{1.0f,0.0f},0.2f} });
	// ロックオン
	inputCommand_->RegisterCommand("CameraLockOn", { {InputState::KeyTrigger, DIK_L},{InputState::PadTriggerRightTrigger,0,{0.0f,0.0f},0.2f},{InputState::PadTriggerLeftTrigger,0,{0.0f,0.0f},0.2f} });

	// AttackDownコマンド
	inputCommand_->RegisterCommand("AttackDown", { {InputState::MouseTrigger, 1}, {InputState::PadTrigger, XINPUT_GAMEPAD_X} });
	// RushChargeコマンド
	inputCommand_->RegisterCommand("RushCharge", { {InputState::MouseTrigger, 1}, {InputState::PadTrigger, XINPUT_GAMEPAD_X} });
	// RushStartコマンド
	inputCommand_->RegisterCommand("RushStart", { {InputState::MouseRelease, 1}, {InputState::PadRelease, XINPUT_GAMEPAD_X} });
}

#include "pch.h"
#include "GameScene.h"
#include "ImguiManager.h"
using namespace GameEngine;

#include "PostProcess/PostEffectData.h"

GameScene::~GameScene() {
}

GameScene::GameScene() {
	// 入力コマンド設定s
	InputRegisterCommand();

	// 背景を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("qwantani_moon_noon_puresky_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);
	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// ライトを設定
	auto* light = renderQueue_->GetLightManager();
	light->directionalLight_->directionalLightData_.intensity = 0.9f;
}

void GameScene::Initialize() {

	
}

void GameScene::Update() {
	mainCamera_->Update();
}

void GameScene::Draw() {
	
}

void GameScene::InputRegisterCommand() {

	// 決定ボタン
	inputCommand_->RegisterCommand("PauseAction", { KeyBinding{ DIK_M, PressType::kTrigger },PadButtonBinding{ XINPUT_GAMEPAD_START, PressType::kTrigger } });
	inputCommand_->RegisterCommand("Decision", { KeyBinding{ DIK_SPACE, PressType::kTrigger },PadButtonBinding{ XINPUT_GAMEPAD_A, PressType::kTrigger } });
	// 入りのムービーを飛ばす
	inputCommand_->RegisterCommand("SkipMovie", { KeyBinding{ DIK_SPACE, PressType::kTrigger },PadButtonBinding{ XINPUT_GAMEPAD_A, PressType::kTrigger } });
	inputCommand_->RegisterCommand("SelectUp", { KeyBinding{ DIK_W, PressType::kTrigger },PadStickBinding{ PadSide::kLeft, {0.0f,1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_UP, PressType::kTrigger } });
	inputCommand_->RegisterCommand("SelectDown", { KeyBinding{ DIK_S, PressType::kTrigger },PadStickBinding{ PadSide::kLeft, {0.0f,-1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_DOWN, PressType::kTrigger } });

	// 移動の入力コマンドを登録する
	inputCommand_->RegisterCommand("MoveUp", { KeyBinding{ DIK_W },PadStickBinding{ PadSide::kLeft, {0.0f,1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveDown", { KeyBinding{ DIK_S },PadStickBinding{ PadSide::kLeft, {0.0f,-1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_DOWN } });
	inputCommand_->RegisterCommand("MoveLeft", { KeyBinding{ DIK_A },PadStickBinding{ PadSide::kLeft, {-1.0f,0.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { KeyBinding{ DIK_D },PadStickBinding{ PadSide::kLeft, {1.0f,0.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_RIGHT } });
	// ジャンプコマンドを登録する
	inputCommand_->RegisterCommand("Jump", { KeyBinding{ DIK_SPACE, PressType::kTrigger },PadButtonBinding{ XINPUT_GAMEPAD_A, PressType::kTrigger } });

	// カメラ操作のコマンドを登録する
	inputCommand_->RegisterCommand("CameraMoveLeft", { KeyBinding{ DIK_LEFT },PadStickBinding{ PadSide::kRight, {-1.0f,0.0f} } });
	inputCommand_->RegisterCommand("CameraMoveRight", { KeyBinding{ DIK_RIGHT },PadStickBinding{ PadSide::kRight, {1.0f,0.0f} } });
	inputCommand_->RegisterCommand("CameraMoveUp", { KeyBinding{ DIK_UP },PadStickBinding{ PadSide::kRight, {0.0f,1.0f} } });
	inputCommand_->RegisterCommand("CameraMoveDown", { KeyBinding{ DIK_DOWN },PadStickBinding{ PadSide::kRight, {0.0f,-1.0f} } });
	// ロックオン
	inputCommand_->RegisterCommand("CameraLockOn", { KeyBinding{ DIK_L, PressType::kTrigger },PadTriggerBinding{ PadSide::kRight, PressType::kTrigger },PadTriggerBinding{ PadSide::kLeft, PressType::kTrigger } });

	// AttackDownコマンド
	inputCommand_->RegisterCommand("AttackDown", { MouseBinding{ 1, PressType::kTrigger }, PadButtonBinding{ XINPUT_GAMEPAD_X, PressType::kTrigger } });
	// RushChargeコマンド
	inputCommand_->RegisterCommand("RushCharge", { MouseBinding{ 1, PressType::kTrigger }, PadButtonBinding{ XINPUT_GAMEPAD_X, PressType::kTrigger } });
	// RushStartコマンド
	inputCommand_->RegisterCommand("RushStart", { MouseBinding{ 1, PressType::kRelease }, PadButtonBinding{ XINPUT_GAMEPAD_X, PressType::kRelease } });
}
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
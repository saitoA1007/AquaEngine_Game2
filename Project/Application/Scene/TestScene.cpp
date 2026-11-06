#include "pch.h"
#include "TestScene.h"
#include "ImguiManager.h"
#include "PostProcess/PostEffectData.h"
#include "RandomGenerator.h"
#include "FPSCounter.h"
#include "Application/CollisionConfig.h"
#include "Application/Demo/CellFractureTest.h"
#include <cassert>
using namespace GameEngine;

TestScene::~TestScene() {}

TestScene::TestScene() {

    // 全破片を同時に飛ばす入力を登録する。
    inputCommand_->RegisterCommand("BreakCellFractureCube", { KeyBinding{ DIK_B, PressType::kTrigger } });

	// 決定ボタンコマンドを追加
	inputCommand_->RegisterCommand("Decision", { KeyBinding{ DIK_SPACE, PressType::kTrigger },PadButtonBinding{ XINPUT_GAMEPAD_X, PressType::kTrigger } });
	inputCommand_->RegisterCommand("MoveUp", { KeyBinding{ DIK_F } });
	inputCommand_->RegisterCommand("MoveDown", { KeyBinding{ DIK_G } });
	inputCommand_->RegisterCommand("MoveLeft", { KeyBinding{ DIK_A },PadStickBinding{ PadSide::kLeft, {-1.0f,0.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { KeyBinding{ DIK_D },PadStickBinding{ PadSide::kLeft, {1.0f,0.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_RIGHT } });
	inputCommand_->RegisterCommand("MoveForward", { KeyBinding{ DIK_W },PadStickBinding{ PadSide::kLeft, {0.0f,1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveBack", { KeyBinding{ DIK_S },PadStickBinding{ PadSide::kLeft, {0.0f,-1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_DOWN } });
	// 破壊オブジェクトを元の姿へ戻す
	inputCommand_->RegisterCommand("Reassemble", { KeyBinding{ DIK_R, PressType::kTrigger } });

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,1.0f,-15.0f} }, 1280, 720);
	mainCamera_->Update();

	// 背景画像を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// プレイヤーモデルを生成
	model_ = modelManager_->GetNameByModel("walk.gltf");
	model_->SetDefaultIsEnableLight(true);
	model_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	world_.Initialize({ {2.0f,2.0f,2.0f},{0.0f,0.0f,0.0f},{2.0f,-1.0f,0.0f} });

	// アニメーションデータを取得する
	walkAnimationData_ = animationManager_->GetNameByAnimations("Walk");
	// アニメーションの再生を管理する
	walkAnimator_ = std::make_unique<Animator>();
	walkAnimator_->Initialize(model_, &walkAnimationData_["Armature|mixamo.com|Layer0"]);

	// 地面
	terrainModel_ = modelManager_->GetNameByModel("terrain.obj");
	terrainModel_->SetDefaultIsEnableLight(true);
	terrainModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	uint32_t grassGH = textureManager_->GetHandleByName("grass.png");
	terrainModel_->SetDefaultTextureHandle(grassGH);
	terrainWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,-4.0f,0.0f} });
	// ノーマルマップを設定
	uint32_t normalGH = textureManager_->GetHandleByName("testNormal.png");
	terrainModel_->SetDefaultNormalTexture(normalGH);

	// テキスト
	text_ = std::make_unique<Text>(fontManager_->GetFont("源直ゴシック EMG 2 - Medium"), "馬力強い馬にマタガーリ");
	text_->Update();

    // 専用モデルを取得し、生成・更新・描画は管理クラスへ任せる。
    Model* fractureModel = modelManager_->GetNameByModel("CellFractureCube.gltf");
    assert(fractureModel != nullptr);
    gameObjectManager_->AddObject<CellFractureTest>(fractureModel, inputCommand_);
}

void TestScene::Initialize() {

}

void TestScene::Update() {

	// カメラの更新処理
	mainCamera_->Update();

	world_.UpdateTransformMatrix();

	// アニメーションの更新処理
	walkAnimator_->ComputeUpdate();

	text_->Update();

	DebugUpdate();
}

void TestScene::DebugUpdate() {
#ifdef USE_IMGUI
	auto* light = renderQueue_->GetLightManager();

	ImGui::Begin("test");

	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	ImGui::ColorEdit4("lightColor", &lightColor_.x);

	ImGui::End();
#endif
}

void TestScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 地面を描画
	renderQueue_->SubmitRaytracingModel(terrainModel_, terrainWorld_);

	// テキストを描画
	renderQueue_->SubmitText(text_.get());

	// アニメーションモデル
	renderQueue_->SubmitRaytracingModel(model_, world_);
}

#include "pch.h"
#include "TestScene.h"
#include "ImguiManager.h"
#include "PostProcess/PostEffectData.h"
#include "RandomGenerator.h"
#include "FPSCounter.h"
#include "Application/CollisionConfig.h"
#include "Application/Demo/IceDemo.h"
#include "Application/Demo/FructureDemo.h"
#include "Application/Demo/PBRDemo.h"
using namespace GameEngine;

TestScene::~TestScene() {}

TestScene::TestScene() {

    // 決定ボタンコマンドを追加
	inputCommand_->RegisterCommand("Decision", { {InputState::KeyTrigger, DIK_SPACE},{InputState::PadTrigger, XINPUT_GAMEPAD_X} });
	inputCommand_->RegisterCommand("MoveUp", { {InputState::KeyPush, DIK_F }});
	inputCommand_->RegisterCommand("MoveDown", { {InputState::KeyPush, DIK_G }});
	inputCommand_->RegisterCommand("MoveLeft", { {InputState::KeyPush, DIK_A },{InputState::PadLeftStick,0,{-1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { {InputState::KeyPush, DIK_D },{InputState::PadLeftStick,0,{1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_RIGHT } });
	inputCommand_->RegisterCommand("MoveForward", { {InputState::KeyPush, DIK_W },{InputState::PadLeftStick,0,{0.0f,1.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveBack", { {InputState::KeyPush, DIK_S },{InputState::PadLeftStick,0,{0.0f,-1.0f},0.2f}, {InputState::PadPush, XINPUT_GAMEPAD_DPAD_DOWN} });
	// 破壊オブジェクトを元の姿へ戻す
	inputCommand_->RegisterCommand("Reassemble", { {InputState::KeyTrigger, DIK_R } });

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

	uint32_t normalGH = textureManager_->GetHandleByName("testNormal.png");
	terrainModel_->SetDefaultNormalTexture(normalGH);

	// エフェクト用モデル
	effectModel_ = modelManager_->GetNameByModel("plane.obj");
	effectModel_->SetDefaultIsEnableLight(false);
	//gameObjectManager_->AddObject<ParticleBehavior>("HitAfterEffect", 32, textureManager_, effectModel_, &renderQueue_->GetMainCamera());
	//gpuParticle_ = gameObjectManager_->AddObject<ParticleBehaviorGPU>("GpuParticle", 1024, effectModel_);

	//gameObjectManager_->AddObject<ParticleBehavior>("BossHitStarParticle", 8, textureManager_, effectModel_);
	//gameObjectManager_->AddObject<ParticleBehavior>("BossHitLineParticle", 32, textureManager_, effectModel_);
	//gameObjectManager_->AddObject<ParticleBehavior>("BossHitParticle", 32, textureManager_, effectModel_);
	 
	//gameObjectManager_->AddObject<ParticleBehavior>("PlayerDownAttackParticle", 128, textureManager_, effectModel_);
	//gameObjectManager_->AddObject<ParticleBehavior>("PlayerDownTrailParticle", 128, textureManager_, effectModel_);
	 
	//gameObjectManager_->AddObject<ParticleBehavior>("tParticle", 128, textureManager_, effectModel_);
	//gameObjectManager_->AddObject<ParticleBehavior>("PlayerHitLineParticle", 32, textureManager_, effectModel_);
	//gameObjectManager_->AddObject<ParticleBehavior>("PlayerHitBoxParticle", 32, textureManager_, effectModel_);
	//gameObjectManager_->AddObject<ParticleBehavior>("PlayerHitCircleParticle", 32, textureManager_, effectModel_);

	// ボスのラッシュ演出
	auto* waModel = modelManager_->GetNameByModel("rushWave.obj");
	waModel->SetDefaultIsEnableLight(false);
	//gameObjectManager_->AddObject<ParticleBehavior>("PlayerLandingEffect", 32, textureManager_, waModel);
	//gameObjectManager_->AddObject<ParticleBehavior>("PlayerChargeParticle", 32, textureManager_, waModel);

	// 中ポリゴン氷
	iceMiddleModel_ = modelManager_->GetNameByModel("ice_middlePolygon.gltf");
	iceMiddleModel_->SetDefaultIsEnableLight(true);
	iceMiddleModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,0.9f });
	iceMiddleModel_->SetDefaultIOR(1.309f);
	iceMiddleWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,2.0f,0.0f} });

	// キューブ氷
	iceCubeModel_ = modelManager_->GetNameByModel("cube.obj");
	iceCubeModel_->SetDefaultIsEnableLight(true);
	iceCubeModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,0.9f });
	iceCubeModel_->SetDefaultIOR(1.309f);
	iceCubeWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{8.0f,2.0f,0.0f} });

	// 破片のテスト
	testModel_ = modelManager_->GetNameByModel("test.gltf");

	// 1つに集約していない破片
	noFractureModel_ = modelManager_->GetNameByModel("NoCellTestFracture.gltf");
	noFractureWorld_.transform_.translate.x = 3.0f;
	noFractureWorld_.UpdateTransformMatrix();

	// 色調補正
	//auto* colorGrading = postEffectManager_->GetPostEffect<ColorGrading>("ColorGradingPass");
	//colorGrading->SetEnableGrayscale(true);

	// 氷のデモ
	gameObjectManager_->AddObject<IceDemo>("IceDemo", iceMiddleModel_);

	// 破片のデモ
	//gameObjectManager_->AddObject<FructureDemo>("FructureDemo", inputCommand_, testModel_);

	// PBRのデモ
	auto* sphereModel = modelManager_->GetNameByModel("sphereShade.obj");
	gameObjectManager_->AddObject<PBRDemo>(sphereModel);

	text_ = std::make_unique<Text>(fontManager_->GetFont("源直ゴシック EMG 2 - Medium"), "馬力強い馬にマタガーリ");
}

void TestScene::Initialize() {

}

void TestScene::Update() {

	if (inputCommand_->IsCommandActive("Decision")) { 
		//isFinished_ = true;
		//auto* colorGrading = postEffectManager_->GetPostEffect<ColorGrading>("ColorGradingPass");
		//colorGrading->SetEnableGrayscale(false);
	}

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

	ImGui::DragFloat3("PlayerPos", &world_.transform_.translate.x, 0.1f);
	ImGui::DragFloat3("PlayerScale", &world_.transform_.scale.x, 0.1f);
	ImGui::ColorEdit4("PlayerColor", &playerColor_.x);

	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	ImGui::ColorEdit4("lightColor", &lightColor_.x);

	ImGui::DragFloat2("tPos", &text_->position_.x, 0.1f);
	ImGui::DragFloat2("tScale", &text_->scale_.x, 0.1f);
	ImGui::DragFloat("tRot", &text_->rotate_, 0.1f);
	ImGui::DragFloat2("tAnchor", &text_->anchorPoint_.x, 0.1f);
	ImGui::ColorEdit4("tColor", &text_->color_.x);

	dir_.Normalize();

	light->SetDirectionalDirction(dir_);
	light->SetDirectionalIntensity(intensity_);
	light->SetDirectionalColor(lightColor_);
	world_.UpdateTransformMatrix();
	model_->SetDefaultColor(playerColor_);
	ImGui::End();


	//ImGui::Begin("IceMaterial");
	//ImGui::ColorEdit4("IceColor", &iceMaterial_.materialData_->color.x);
	//ImGui::DragFloat("IceRoughness", &iceMaterial_.materialData_->roughness, 0.01f, 0.0f, 1.0f);
	//ImGui::DragFloat("IceIor", &iceMaterial_.materialData_->ior, 0.01f);
	//
	//ImGui::Separator();
	//
	//ImGui::DragFloat("IceChipScale", &iceMaterial_.materialData_->chipScale, 0.01f);
	//ImGui::DragFloat("IceChipStrength", &iceMaterial_.materialData_->chipStrength, 0.01f, 0.0f, 1.0f);
	//ImGui::DragFloat("IceEdgeWidth", &iceMaterial_.materialData_->edgeWidth, 0.01f);
	//ImGui::DragFloat("IceEdgeStrength", &iceMaterial_.materialData_->edgeStrength, 0.01f, 0.0f, 1.0f);
	//ImGui::DragFloat("IceMicroScale", &iceMaterial_.materialData_->microScale, 0.01f);
	//ImGui::DragFloat("IceMicroStrength", &iceMaterial_.materialData_->microStrength, 0.01f);
	//ImGui::DragFloat("IceDissolveThreshold", &iceMaterial_.materialData_->dissolveThreshold, 0.01f,0.0f,1.0f);
	//
	//ImGui::Separator();
	//
	//ImGui::DragFloat("bubbleScale", &iceMaterial_.materialData_->bubbleScale, 0.01f, 0.0f, 1.0f);
	//ImGui::DragFloat("bubbleMaxDepth", &iceMaterial_.materialData_->bubbleMaxDepth, 0.01f, 0.0f,10.0f);
	//ImGui::DragFloat("bubbleDensity", &iceMaterial_.materialData_->bubbleDensity, 0.01f, 0.0f, 1.0f);
	//ImGui::DragFloat("bubbleJitter", &iceMaterial_.materialData_->bubbleJitter, 0.01f, 0.0f, 1.0f);
	//ImGui::DragFloat("bubbleHighlight", &iceMaterial_.materialData_->bubbleHighlight, 0.01f, 0.0f, 1.0f);
	//ImGui::End();
#endif
}

void TestScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 地面を描画
	renderQueue_->SubmitRaytracingModel(terrainModel_, terrainWorld_);

	//renderQueue_->SubmitText(text_.get());

	// 破片を1つに集約していない
	//renderQueue_->SubmitModel(noFractureModel_, noFractureWorld_);

	// アニメーションモデル
	//renderQueue_->SubmitRaytracingModel(model_, world_);
}

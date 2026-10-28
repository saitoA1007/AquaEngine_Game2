#include "pch.h"
#include "DemoScene.h"
#include "ImguiManager.h"
#include "PostProcess/PostEffectData.h"
#include "FPSCounter.h"
#include "ParticleBehavior.h"
#include "Application/Demo/IceDemo.h"
#include "Application/Demo/FructureDemo.h"
#include "Application/Demo/PBRDemo.h"
#include "Application/Demo/Test.h"
#include "Application/Demo/TestFructure.h"
using namespace GameEngine;

DemoScene::~DemoScene() {}

DemoScene::DemoScene() {

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,1.0f,-15.0f} }, 1280, 720);
	mainCamera_->Update();

	debugParame_.Register("EaseCurve", easeCurve_);
	debugParame_.Register("MaxTime", maxTime_,1);
	debugParame_.Apply();

	// 背景画像を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// エフェクト用モデル
	auto* effectModel = modelManager_->GetNameByModel("plane.obj");
	effectModel->SetDefaultIsEnableLight(false);
	gameObjectManager_->AddObject<ParticleBehavior>("DemoParticle", 64, textureManager_, effectModel);
	gameObjectManager_->AddObject<ParticleBehavior>("Demo2Particle", 64, textureManager_, effectModel);
	gameObjectManager_->AddObject<ParticleBehavior>("Demo3Particle", 64, textureManager_, effectModel);
	//gameObjectManager_->AddObject<ParticleBehavior>("PlayerHitRingParticle", 8, textureManager_, effectModel);
	
	// オブジェクトのテスト
	//auto* testModel = modelManager_->GetNameByModel("sphereShade.obj");
	//gameObjectManager_->AddObject<Test>(testModel);

	// 破片のテスト
	auto* testModel_ = modelManager_->GetNameByModel("test.gltf");
	// 破片のデモ
	gameObjectManager_->AddObject<FructureDemo>("FructureDemo", inputCommand_, testModel_);

	// PBRのデモ
	auto* sphereModel = modelManager_->GetNameByModel("sphereShade.obj");
	gameObjectManager_->AddObject<PBRDemo>(sphereModel);

	// テキスト
	text_ = std::make_unique<Text>(fontManager_->GetFont("源直ゴシック EMG 2 - Medium"), "テスト");
	text_->Update();
}

void DemoScene::Initialize() {

}

void DemoScene::Update() {
	debugParame_.ApplyIfDirty();

	timer_ += FpsCounter::deltaTime / maxTime_;

	//text_->color_.w = Lerp(0.0f, 1.0f, timer_, easeCurve_);

	if (timer_ >= 1.0f) {
		timer_ = 0.0f;
	}

	if (inputCommand_->IsCommandActive("Decision")) {
		//isFinished_ = true;
		//auto* colorGrading = postEffectManager_->GetPostEffect<ColorGrading>("ColorGradingPass");
		//colorGrading->SetEnableGrayscale(false);
	}

	// カメラの更新処理
	mainCamera_->Update();

	text_->Update();

	DebugUpdate();
}

void DemoScene::DebugUpdate() {
#ifdef USE_IMGUI
	auto* light = renderQueue_->GetLightManager();

	ImGui::Begin("test");
	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	ImGui::ColorEdit4("lightColor", &lightColor_.x);

	ImGui::DragFloat2("tPos", &text_->position_.x, 0.1f);
	ImGui::DragFloat2("tScale", &text_->scale_.x, 0.1f);
	ImGui::DragFloat("tRot", &text_->rotate_, 0.1f);
	ImGui::DragFloat2("tAnchor", &text_->anchorPoint_.x, 0.1f);
	ImGui::ColorEdit4("tColor", &text_->color_.x);

	if (ImGui::InputText("Name", buf, sizeof(buf))) {
		textName_ = buf; // 入力が変更されたらstd::stringを更新
		text_->SetText(textName_);
		text_->Update();
	}

	dir_.Normalize();

	light->SetDirectionalDirction(dir_);
	light->SetDirectionalIntensity(intensity_);
	light->SetDirectionalColor(lightColor_);
	ImGui::End();
#endif
}

void DemoScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	renderQueue_->SubmitText(text_.get());

	// 破片を1つに集約していない
	//renderQueue_->SubmitModel(noFractureModel_, noFractureWorld_);

	// アニメーションモデル
	//renderQueue_->SubmitRaytracingModel(model_, world_);
}

#include "pch.h"
#include "ResourceSubsystem.h"
#include "DebugParameter.h"
#include "GraphicsSubsystem.h"
#include "ScopedTimer.h"
using namespace GameEngine;

ResourceSubsystem::ResourceSubsystem() = default;
ResourceSubsystem::~ResourceSubsystem() = default;

void ResourceSubsystem::Initialize() {
	auto* cmdList = context_.graphics->GetGraphicsDevice()->GetCommandList();
	auto* srvManager = context_.graphics->GetGraphicsDevice()->GetSrvManager();

	// テクスチャの初期化
	textureManager_ = std::make_unique<TextureManager>();
	textureManager_->Initialize(cmdList, srvManager);

	// フォントの初期化
	fontManager_ = std::make_unique<FontManager>();
	fontManager_->Initialize(cmdList);

	// モデルを管理するクラスを生成
	modelManager_ = std::make_unique<ModelManager>();
	modelManager_->Initialize(cmdList, textureManager_.get(), srvManager);

	// アニメーション
	animationManager_ = std::make_unique<AnimationManager>();

	// 音声
	AudioManager::GetInstance().Initialize();

	// パラメータファイルの読み込み
	gameParamEditor_ = std::make_unique<GameParamEditor>();
	DebugParameter::StaticInitialize(gameParamEditor_.get());

	// エフェクトの管理
	effectsManager_ = std::make_unique<EffectsManager>(textureManager_.get(), modelManager_.get());

	// 全てのリソースをロードする
	LoadAllResources();
}

void ResourceSubsystem::LoadAllResources() {
	ScopedTimer total("LoadAllResources (Total)");

	{
		ScopedTimer t("ParamFiles");
		// 全てのパラメータファイルを読み込み
		gameParamEditor_->LoadFiles();
	}

	// GPUを使わない処理を先に背景で開始する
	modelManager_->BeginLoadAllModel();

	{
		ScopedTimer t("Texture");
		// 画像データを全てロードする
		textureManager_->LoadAllTexture();
#ifdef USE_IMGUI
		// エディターのみで使用する画像をロード
		textureManager_->LoadDebugAllTexture();
#endif
	}

	{
		ScopedTimer t("Font");
		// フォントデータを全てロードする
		fontManager_->LoadAllFont();
	}
	
	{
		ScopedTimer t("Animation");
		// AnimationsRegister.jsonに登録されたアニメーションデータを全てロードする
		animationManager_->LoadAllAnimation();
	}
	
	{
		ScopedTimer t("Audio");
		// 音声データを全てロードする
		AudioManager::GetInstance().LoadAllAudio();
	}

	{
		ScopedTimer t("Effects");
		// エフェクトデータを全てロードする
		effectsManager_->Initialize();
	}

	{
		ScopedTimer t("Model (wait + build)");
		// モデルデータを全てロードする
		modelManager_->FinishLoadAllModel();
		modelManager_->RegisterGridPlaneModel("Grid", { 200.0f, 200.0f });
		modelManager_->RegisterRingModel("Ring", 32, 1.0f, 0.2f);
		modelManager_->RegisterCylinderModel("Cylinder", 32, 1.0f, 1.0f, 3.0f);

		// モデルに実際にテクスチャを適応させる
		modelManager_->ApplyTexutre(textureManager_.get());
	}
}

void ResourceSubsystem::Finalize() {
	AudioManager::GetInstance().Finalize();
	fontManager_->Finalize();
	textureManager_->Finalize();
}


#include "pch.h"
#include "TitleScene.h"
using namespace GameEngine;

#include "PostProcess/PostEffectData.h"

TitleScene::TitleScene() {
	// 入力コマンド設定
	InputRegisterCommand();

	// アセットを取得する
	LoadAssets();

	// 3Dの物体がないタイトルではフォグが使えないため、タイトルの間だけ切る
	DisableFog();
}

TitleScene::~TitleScene() {
	// 次のシーンでは元どおりフォグが掛かるようにする（シーンは描画の機能より先に破棄される）
	RestoreFog();
}

void TitleScene::Initialize() {

	// エンジンのリセットは、オブジェクトを消さずにInitializeをもう一度呼ぶ。二重にならないよう、先に全て削除する
	gameObjectManager_->ClearAll();

	// 描画に使うカメラを設定する
	SetUpEnvironment();

	// タイトルの表示を生成する。更新・描画・削除はGameObjectManagerへ任せる
	titleView_ = gameObjectManager_->AddObject<TitleView>(fontManager_, whiteTextureHandle_);
}

void TitleScene::Update() {
	// 決定の入力（押した瞬間）でシーンを終え、フェードを挟んでGameシーンへ進む
	if (inputCommand_->IsCommandActive("Decide")) {
		isFinished_ = true;
	}
}

void TitleScene::LoadAssets() {
	// 背景の板に使う白いテクスチャを取得する。
	// エンジンが最初に読み込む画像なので必ず存在し、ハンドルは0になりうる（0を失敗と判定できない）ため確認しない
	whiteTextureHandle_ = textureManager_->GetHandleByName("white2x2.png");
}

void TitleScene::SetUpEnvironment() {
	// 前のシーンのカメラは破棄されているため、このシーンのカメラを設定し直す
	camera_ = std::make_unique<Camera>();
	camera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,1.0f,-15.0f} }, 1280, 720);
	camera_->Update();
	renderQueue_->SetCamera(camera_.get());
}

void TitleScene::DisableFog() {
	// グラフにフォグがなければ、切る必要はない
	Fog* fog = postEffectManager_->GetPostEffect<Fog>(kFogEffectName);
	if (fog == nullptr) {
		return;
	}

	wasFogActive_ = fog->IsActive();
	fog->SetIsActive(false);
}

void TitleScene::RestoreFog() {
	// タイトルの間にエディタでノードが消されても困らないよう、取り直してから戻す
	Fog* fog = postEffectManager_->GetPostEffect<Fog>(kFogEffectName);
	if (fog == nullptr) {
		return;
	}

	fog->SetIsActive(wasFogActive_);
}

void TitleScene::InputRegisterCommand() {

	// 決定（Spaceとパッドの A）。押した瞬間だけ有効にする。GameSceneも同じ名前で登録する
	inputCommand_->RegisterCommand("Decide", { KeyBinding{ DIK_SPACE, PressType::kTrigger }, PadButtonBinding{ XINPUT_GAMEPAD_A, PressType::kTrigger } });
}

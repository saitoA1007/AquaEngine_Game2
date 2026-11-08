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
}

void GameScene::Initialize() {

	// エンジンのリセットは、オブジェクトを消さずにInitializeをもう一度呼ぶ。前回のオブジェクトが残って二重にならないよう、先に全て削除する
	gameObjectManager_->ClearAll();

	// 背景・カメラ・ライトを設定する
	SetUpEnvironment();

	// ゲームオブジェクトを生成する
	CreateGameObjects(cylinderModel_, sphereModel_);

}

void GameScene::Update() {
	debugParam_->ApplyIfDirty();

	// ライトの調整値を反映する
	UpdateLight();

	// 検証用：同時破砕の計測を始める
	if (inputCommand_->IsCommandActive("ShatterBenchmark")) {
		shatterEffect_->StartBenchmark();
	}

	// シーン更新はオブジェクト更新の後に走るため、更新後のプレイヤー位置を敵が使える
	StepEnemies();

	// 敵の更新後の位置で、氷塊の回収と連鎖を進める
	StepIceShards();

	// 押し戻し後のプレイヤー位置を追従する
	gameCamera_->Update(player_->GetPosition());
}

void GameScene::Draw() {

}

void GameScene::LoadAssets() {
	// モデルを取得する
	cylinderModel_ = modelManager_->GetNameByModel("Cylinder");
	sphereModel_ = modelManager_->GetNameByModel("sphere.obj");
	planeModel_ = modelManager_->GetNameByModel("plane.obj");
	shellShardModel_ = modelManager_->GetNameByModel(kShellShardModelName);

	// 背景のテクスチャを取得する
	skyboxTextureHandle_ = textureManager_->GetHandleByName("qwantani_moon_noon_puresky_1k.dds");

	// 氷ゲージの矩形に使う白いテクスチャを取得する
	whiteTextureHandle_ = textureManager_->GetHandleByName("white2x2.png");

	// 取得できなかったモデルがないか確認する
	assert(cylinderModel_ != nullptr);
	assert(sphereModel_ != nullptr);
	assert(planeModel_ != nullptr);
	assert(shellShardModel_ != nullptr);
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

	// 氷霧の発生を受け取る。氷霧の見た目と破片は敵より先に作る
	iceMistEffect_ = gameObjectManager_->AddObject<IceMistEffect>(planeModel_, textureManager_);
	shatterEffect_ = gameObjectManager_->AddObject<ShatterEffectManager>(shellShardModel_);
	player_->SetIceMistCallback([this](const IceMistInfo& info) { OnIceMist(info); });

	// 敵（敵一覧とモデルの読み込みに失敗すると例外を投げる）
	enemyManager_ = gameObjectManager_->AddObject<EnemyManager>(modelManager_, field_->GetRadius(), gameCamera_->GetCamera(), whiteTextureHandle_, shatterEffect_);

	// 氷塊（敵より後に作る）。撃破した敵の位置に氷塊を落とす
	iceShardManager_ = gameObjectManager_->AddObject<IceShardManager>(sphereModel, enemyManager_);
	enemyManager_->SetDefeatCallback([this](const EnemyDefeatInfo& info) { iceShardManager_->Drop(info.position, info.iceShardCount); });
}

void GameScene::OnIceMist(const IceMistInfo& info) {
	// 処理の順番はここで決める（更新順には頼らない）。敵へ当て、氷塊へ通知し、見た目を出す。氷霧で撃破した敵の氷塊は地面に落ち、この発射には含まれない
	enemyManager_->ApplyIceMist(info);
	NotifyIceShards(info);
	iceMistEffect_->Play(info.center, info.radius);

}

void GameScene::NotifyIceShards(const IceMistInfo& info) {
	// 氷霧の範囲に合わせて、持っている氷塊を連鎖させて飛ばす
	iceShardManager_->Launch(info);
}

void GameScene::StepIceShards() {
	// 氷塊が見るプレイヤーの情報をまとめる
	IceShardInput input;
	input.playerPosition = player_->GetPosition();
	input.isCharging = player_->IsCharging();

	iceShardManager_->UpdateShards(input);
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

	// 移動の入力コマンドを登録する
	inputCommand_->RegisterCommand("MoveUp", { KeyBinding{ DIK_W },PadStickBinding{ PadSide::kLeft, {0.0f,1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveDown", { KeyBinding{ DIK_S },PadStickBinding{ PadSide::kLeft, {0.0f,-1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_DOWN } });
	inputCommand_->RegisterCommand("MoveLeft", { KeyBinding{ DIK_A },PadStickBinding{ PadSide::kLeft, {-1.0f,0.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { KeyBinding{ DIK_D },PadStickBinding{ PadSide::kLeft, {1.0f,0.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_RIGHT } });

	// Aボタン（仮の割り当て：Spaceとパッドの A）。押している間のチャージと、離した瞬間の発生に分ける
	inputCommand_->RegisterCommand("IceMistCharge", { KeyBinding{ DIK_SPACE, PressType::kPush }, PadButtonBinding{ XINPUT_GAMEPAD_A, PressType::kPush } });
	inputCommand_->RegisterCommand("IceMistRelease", { KeyBinding{ DIK_SPACE, PressType::kRelease }, PadButtonBinding{ XINPUT_GAMEPAD_A, PressType::kRelease } });

	// 同時破砕の計測の開始（検証用）
	inputCommand_->RegisterCommand("ShatterBenchmark", { KeyBinding{ DIK_F9, PressType::kTrigger } });

}

#include "pch.h"
#include "GameScene.h"
#include "ImguiManager.h"
using namespace GameEngine;

#include "PostProcess/PostEffectData.h"
#include "FPSCounter.h"
#include <cassert>

GameScene::~GameScene() {
	// 止めたまま次のシーンへ進まないよう、ゲームの時間を通常へ戻す
	FpsCounter::SetTimeScale(1.0f);
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

	// ゲームオーバーの状態を戻す
	isGameOver_ = false;

	// 振動を作り直す。前回の振動は破棄時に止まる
	padVibration_ = std::make_unique<PadVibration>(inputCommand_);


	// 背景・カメラ・ライトを設定する
	SetUpEnvironment();

	// ゲームオブジェクトを生成する
	CreateGameObjects(cylinderModel_, sphereModel_);

	// 前回のリセット時にゲームオーバーで止まっていても、タイムスケールを1へ戻す
	UpdateTimeScale();
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

	// HPが0ならゲームオーバーに入る。ゲームオーバー中は再開の入力を見る
	CheckGameOver();
	UpdateGameOver();

	// 止めるかどうかをタイムスケールへ反映する
	UpdateTimeScale();

	// 長押しの振動を伝え、そのフレームの振動を出す（氷霧と命中は発生時に要求済み）。ゲームオーバー中は振動を止めたままにする
	if (!IsGameStopped()) {
		padVibration_->SetCharging(player_->IsCharging());
		padVibration_->Update();
	}

	// 押し戻し後のプレイヤー位置を追従する
	gameCamera_->Update(player_->GetPosition());

	// 表示用の値を最後にまとめて渡す（オブジェクトの更新はシーンの更新より先に走るため、描画の直前に最新になる）
	UpdateHud();
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

	// HIT表示の氷の破裂の画像を取得する（Resources/Textures/UI/Combo）
	comboBurstTextureHandle_ = textureManager_->GetHandleByName("ComboBurst.png");

	// HPアイコン（3D）のモデルを取得する。破片モデルは核を8個に分けたもの（_cellの破壊グループが1つ）
	hpHudModels_.frame = modelManager_->GetNameByModel("hpFrame.gltf");
	hpHudModels_.core = modelManager_->GetNameByModel("hpCore.gltf");
	hpHudModels_.shard = modelManager_->GetNameByModel("hpCoreShard.gltf");

	// 自機の核と枠は、HPアイコンと同じモデルを使う
	playerViewAssets_.core = hpHudModels_.core;
	playerViewAssets_.frame = hpHudModels_.frame;

	// 氷エリアのモデルと画像を取得する（Resources/Models/IceArea、Resources/Textures/IceArea）
	iceAreaAssets_.slab = modelManager_->GetNameByModel("iceAreaSlab.gltf");
	iceAreaAssets_.plane = modelManager_->GetNameByModel("planeXZ.obj");
	iceAreaAssets_.slabTexture = textureManager_->GetHandleByName("iceAreaSlabStylized.png");
	iceAreaAssets_.slabHighlight = textureManager_->GetHandleByName("iceAreaSlabHighlight.png");
	iceAreaAssets_.rimLine = textureManager_->GetHandleByName("iceAreaRimLine.png");
	iceAreaAssets_.snowflake = textureManager_->GetHandleByName("iceAreaSnowflake.png");


	// 取得できなかったモデルがないか確認する
	assert(cylinderModel_ != nullptr);
	assert(sphereModel_ != nullptr);
	assert(planeModel_ != nullptr);
	assert(shellShardModel_ != nullptr);
	assert(hpHudModels_.frame != nullptr);
	assert(hpHudModels_.core != nullptr);
	assert(hpHudModels_.shard != nullptr);
	assert(playerViewAssets_.core != nullptr);
	assert(playerViewAssets_.frame != nullptr);
	assert(iceAreaAssets_.slab != nullptr);
	assert(iceAreaAssets_.plane != nullptr);

	// 画像は取得できないと0が返るため、0でないことを確認する
	assert(iceAreaAssets_.slabTexture != 0);
	assert(iceAreaAssets_.slabHighlight != 0);
	assert(iceAreaAssets_.rimLine != 0);
	assert(iceAreaAssets_.snowflake != 0);
	assert(comboBurstTextureHandle_ != 0);
}

void GameScene::RegisterDebugParameters() {
	// サブグループ名。ここを書き換えれば登録の全てに反映される
	const std::string kLightGroup = "Light";
	const std::string kPlayerStatsGroup = "PlayerStats";

	debugParam_ = std::make_unique<DebugParameter>("GameScene");

	// ライト
	debugParam_->Register("lightIntensity", lightIntensity_, 0, kLightGroup);
	debugParam_->Register("lightDirection", lightDirection_, 1, kLightGroup);

	// プレイヤーの固定の値
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
	player_ = gameObjectManager_->AddObject<Player>(playerViewAssets_, iceAreaAssets_, inputCommand_, field_, &playerStats_);

	// 氷霧の発生を受け取る。氷霧の見た目と破片は敵より先に作る
	iceMistEffect_ = gameObjectManager_->AddObject<IceMistEffect>(planeModel_, textureManager_, whiteTextureHandle_);
	shatterEffect_ = gameObjectManager_->AddObject<ShatterEffectManager>(shellShardModel_);
	player_->SetIceMistCallback([this](const IceMistInfo& info) { OnIceMist(info); });

	// 敵（敵一覧とモデルの読み込みに失敗すると例外を投げる）
	enemyManager_ = gameObjectManager_->AddObject<EnemyManager>(modelManager_, field_->GetRadius(), gameCamera_->GetCamera(), whiteTextureHandle_, shatterEffect_);

	// 氷塊（敵より後に作る）。撃破した敵の位置に氷塊を落とす
	iceShardManager_ = gameObjectManager_->AddObject<IceShardManager>(sphereModel, planeModel_, textureManager_, enemyManager_);
	// 経験値とフェーズ（敵一覧を読んだ後に作る）。フェーズの出現ルールの変更を敵へ渡す
	phaseDirector_ = gameObjectManager_->AddObject<PhaseDirector>(enemyManager_->GetDefinitions());
	phaseDirector_->SetRuleChangedCallback([this]() { ApplySpawnRule(); });
	ApplySpawnRule();

	// 撃破の通知で、氷塊を落とし、経験値を足し、連鎖数を数える
	enemyManager_->SetDefeatCallback([this](const EnemyDefeatInfo& info) {
		iceShardManager_->Drop(info.position, info.iceShardCount);
		phaseDirector_->AddExp(info.exp);
		comboPopupView_->AddHit(info.chainId, info.position);
		});

	// 氷塊が敵に命中するたびに振動させる
	iceShardManager_->SetHitCallback([this]() { padVibration_->PlayHit(); });

	// 常時表示のUIと連鎖数
	gameHud_ = gameObjectManager_->AddObject<GameHud>(fontManager_, gameCamera_->GetCamera(), hpHudModels_, whiteTextureHandle_);
	comboPopupView_ = gameObjectManager_->AddObject<ComboPopupView>(gameCamera_->GetCamera(), fontManager_, comboBurstTextureHandle_);


	// ゲームオーバーの表示（一番手前に重ねる）
	gameOverView_ = gameObjectManager_->AddObject<GameOverView>(inputCommand_, fontManager_, whiteTextureHandle_);
}

void GameScene::ApplySpawnRule() {
	// フェーズ表の現在の行を、敵が受け取るルールへ写す
	const PhaseRow& row = phaseDirector_->GetCurrentRow();
	EnemySpawnRule rule;
	rule.interval = row.spawnInterval;
	rule.perInterval = row.spawnPerInterval;
	rule.maxEnemies = row.maxEnemies;
	rule.weights = row.weights;
	enemyManager_->SetSpawnRule(rule);
}

bool GameScene::IsGameStopped() const {
	// ゲームオーバー中は、ゲームの世界を止める。ここに条件を足していく
	return isGameOver_;
}

void GameScene::UpdateTimeScale() {
	// 止めている間はゲームの世界を止める。UIの動きは止まらない時間で進む
	FpsCounter::SetTimeScale(IsGameStopped() ? 0.0f : 1.0f);
}

void GameScene::CheckGameOver() {
	if (isGameOver_ || player_->GetHp() > 0) {
		return;
	}

	// 止まっている間にAを離して氷霧が出ないよう入力を切り、振動も止めてから、表示を開く
	isGameOver_ = true;
	player_->SetInputEnabled(false);
	padVibration_->Stop();
	gameOverView_->Open();
}

void GameScene::UpdateGameOver() {
	if (!isGameOver_ || !gameOverView_->IsRetryRequested()) {
		return;
	}

	// シーンを終えると、フェードを挟んでGameシーンが最初から作り直される
	isFinished_ = true;
}

void GameScene::UpdateHud() {
	// 表示用の値をまとめる
	HudInput input;
	input.hp = player_->GetHp();
	input.maxHp = player_->GetMaxHp();
	input.phase = phaseDirector_->GetPhase();
	gameHud_->SetInput(input);

}

void GameScene::OnIceMist(const IceMistInfo& info) {
	// 処理の順番はここで決める（更新順には頼らない）。敵へ当て、氷塊へ通知し、見た目を出す。氷霧で撃破した敵の氷塊は地面に落ち、この発射には含まれない
	enemyManager_->ApplyIceMist(info);
	NotifyIceShards(info);
	iceMistEffect_->Play(info.center, info.radius);

	// 離して氷霧を発生させた振動。単押しも長押しも同じ
	padVibration_->PlayRelease();
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

	// 飛んでいる氷塊の数を自機の見た目へ伝える（連鎖中は回転が速くなる）。Playerは氷塊の管理を知らない
	player_->SetFlyingShardCount(iceShardManager_->GetFlyingCount());
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

	// 結果をプレイヤーへ反映する。実際にHPが減ったときだけ、被弾の演出と振動を出す
	if (player_->TakeDamage(output.damage)) {
		gameHud_->PlayDamage();
		padVibration_->PlayDamage();
	}
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

	// ゲームオーバーからの再開（Spaceとパッドの A）。押した瞬間だけ有効にする
	inputCommand_->RegisterCommand("Retry", { KeyBinding{ DIK_SPACE, PressType::kTrigger }, PadButtonBinding{ XINPUT_GAMEPAD_A, PressType::kTrigger } });


	// 同時破砕の計測の開始（検証用）
	inputCommand_->RegisterCommand("ShatterBenchmark", { KeyBinding{ DIK_F9, PressType::kTrigger } });

}

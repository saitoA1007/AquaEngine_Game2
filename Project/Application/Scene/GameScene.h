#pragma once
#include "IScene.h"

// エンジン機能をインクルード
#include "Application/Game/Camera/GameCamera.h"
#include "DebugParameter.h"

#include "Application/Scene/Transition/Fade.h"
#include "Application/Game/Player/PlayerStats.h"
#include "Application/Game/Player/Player.h"
#include "Application/Game/Field/Field.h"
#include "Application/Game/Enemy/EnemyManager.h"
#include "Application/Game/Effect/ShatterEffectManager.h"
#include "Application/Game/Effect/IceMistEffect.h"
#include "Application/Game/IceShard/IceShardManager.h"
#include "Application/Game/Player/PadVibration.h"
#include "Application/Game/Phase/PhaseDirector.h"
#include "Application/Game/Upgrade/PlayerUpgrade.h"
#include "Application/Game/Upgrade/UpgradeSelectUI.h"
#include "Application/Game/UI/GameHud.h"
#include "Application/Game/UI/GameOverView.h"
#include "Application/Game/UI/ComboPopupView.h"

class GameScene : public GameEngine::IScene {
public:
	GameScene();
	~GameScene();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="input"></param>
	void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// デバック時、処理して良いものを更新する
	/// </summary>
	void DebugUpdate() override {}

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw() override;

	/// <summary>
	/// 次のシーン遷移する場面の名前を取得
	/// </summary>
	/// <returns></returns>
	std::string NextSceneName() override { return "Game"; } // 仮。リザルトとタイトル（C5）ができたら変える。再開はGameシーンを最初からやり直す

	/// <summary>
	/// 遷移する演出
	/// </summary>
	/// <returns></returns>
	std::unique_ptr<ITransitionEffect> GetTransitionEffect() override { return std::make_unique<Fade>(); }

private: // シーン機能

	// ゲーム専用カメラ（調整値と補間はGameCameraが持つ）
	std::unique_ptr<GameCamera> gameCamera_;

private: // アセット

	// 管理クラスが所有するモデルへの非所有ポインタ。LoadAssetsで取得する
	GameEngine::Model* cylinderModel_ = nullptr; // フィールドの地面と縁
	GameEngine::Model* sphereModel_ = nullptr;   // プレイヤーの仮の見た目
	GameEngine::Model* planeModel_ = nullptr;    // 氷霧のパーティクルの板
	GameEngine::Model* shellShardModel_ = nullptr;  // 氷の殻の破片（球の殻を12個に割ったモデル）

	// 背景のテクスチャハンドル
	uint32_t skyboxTextureHandle_ = 0;

	// 氷ゲージの矩形に使う白いテクスチャのハンドル
	uint32_t whiteTextureHandle_ = 0;

	// HPアイコン（3D）のモデル。枠・核・欠片
	HpHudModels hpHudModels_;
	IceAreaAssets iceAreaAssets_;   // 氷エリアの描画に使うモデルとテクスチャ

	// 強化の選択画面で使う画像のハンドル
	UpgradeSelectTextures upgradeSelectTextures_;

private: // ゲームの状態

	// プレイヤーの強化値（Playerが参照する）。強化前の移動速度とPlayerUpgradeの回数から毎フレーム求め直す
	PlayerStats playerStats_;

	// 強化の項目、何回取ったか。シーンが所有し、Initializeのたびに作り直す（回数を0へ戻す）
	std::unique_ptr<PlayerUpgrade> playerUpgrade_;

	// GameObjectManagerが所有するオブジェクトへの非所有ポインタ。シーン終了まで有効
	Player* player_ = nullptr;
	Field* field_ = nullptr;
	EnemyManager* enemyManager_ = nullptr;
	IceShardManager* iceShardManager_ = nullptr;
	PhaseDirector* phaseDirector_ = nullptr;
	UpgradeSelectUI* upgradeSelectUI_ = nullptr;
	GameHud* gameHud_ = nullptr;
	ComboPopupView* comboPopupView_ = nullptr;
	GameOverView* gameOverView_ = nullptr;

	// HPが0になってゲームオーバーの表示中か。Initializeで戻す
	bool isGameOver_ = false;

	// コントローラーの振動。シーンが所有し、Initializeのたびに作り直す（破棄時に振動を止める）
	std::unique_ptr<PadVibration> padVibration_;
	ShatterEffectManager* shatterEffect_ = nullptr;
	IceMistEffect* iceMistEffect_ = nullptr;

private: // 定数

	// 破片モデルの名前（拡張子つき。モデルはファイル名で取得する）
	inline static const char* kShellShardModelName = "IceShellShard12.gltf";

private: // 調整値

	// ライト
	float lightIntensity_ = 0.9f;
	Vector3 lightDirection_ = { -0.3f, -1.0f, 0.4f };

	// 強化前の移動速度（保存する調整値。強化の上昇分は足さない）
	float baseMoveSpeed_ = 8.0f;

	// 調整値の機能
	std::unique_ptr<GameEngine::DebugParameter> debugParam_;

private:

	/// <summary>
	/// 入力のコマンドを設定する
	/// </summary>
	void InputRegisterCommand();

	/// <summary>
	/// モデルやテクスチャを取得する。最後に取得できたかをnullptrで確認する。
	/// </summary>
	void LoadAssets();

	/// <summary>
	/// 調整値をDebugParameterへ登録する。サブグループ名はこの関数内の定数で管理する。
	/// </summary>
	void RegisterDebugParameters();

	/// <summary>
	/// 背景・カメラ・ライトを設定する。調整値の登録後に呼ぶ。
	/// </summary>
	void SetUpEnvironment();

	/// <summary>
	/// ゲームオブジェクトを生成する。更新・描画・削除はGameObjectManagerへ任せる。
	/// </summary>
	/// <param name="cylinderModel">フィールドに使うモデル</param>
	/// <param name="sphereModel">プレイヤーに使うモデル</param>
	void CreateGameObjects(GameEngine::Model* cylinderModel, GameEngine::Model* sphereModel);

	/// <summary>
	/// プレイヤーの情報を敵へ渡して更新し、結果（被弾と押し戻し）をプレイヤーへ反映する。
	/// 構造体はこの関数のローカル変数で、プレイヤーの更新後に呼ぶ。
	/// </summary>
	void StepEnemies();

	/// <summary>
	/// プレイヤーの情報を氷塊へ渡して更新する。敵の更新後に呼ぶ。
	/// </summary>
	void StepIceShards();

	/// <summary>
	/// 氷霧が発生したときの処理順を決める。敵へ当て、氷塊へ通知し、見た目を出す。
	/// </summary>
	/// <param name="info">氷霧の情報</param>
	void OnIceMist(const IceMistInfo& info);

	/// <summary>
	/// 氷塊の管理へ氷霧を通知する呼び出し口。持っている氷塊を連鎖させて発射する。
	/// </summary>
	/// <param name="info">氷霧の情報</param>
	void NotifyIceShards(const IceMistInfo& info);

	/// <summary>
	/// 未処理のレベルアップがあり、選択画面が閉じていれば、選択画面を開いてゲームを止める。
	/// </summary>
	void UpdateLevelUp();

	/// <summary>
	/// 選択画面を開く。プレイヤーの入力と振動を止める。タイムスケールはUpdateTimeScaleが0にする。
	/// </summary>
	void OpenUpgradeSelect();

	/// <summary>
	/// 強化が選ばれたときの処理。強化を反映し、レベルとフェーズを進め、出現ルールを渡す。
	/// 未処理が残っていれば続けて選択画面を開き、なければ入力を戻す。
	/// </summary>
	/// <param name="type">選ばれた強化の項目</param>
	void OnUpgradeSelected(UpgradeType type);

	/// <summary>
	/// 現在のフェーズの出現ルールをEnemyManagerへ渡す。
	/// </summary>
	void ApplySpawnRule();

	/// <summary>
	/// ゲームの世界を止めているかを返す。強化の選択中、またはゲームオーバー中。
	/// </summary>
	bool IsGameStopped() const;

	/// <summary>
	/// ゲームのタイムスケールを決める。FpsCounter::SetTimeScaleを呼ぶのはこの関数だけにする。
	/// 止めているとき（IsGameStopped）は0、それ以外は1。
	/// </summary>
	void UpdateTimeScale();

	/// <summary>
	/// プレイヤーのHPが0ならゲームオーバーに入る。入力と振動を止め、ゲームオーバーの表示を開く。
	/// </summary>
	void CheckGameOver();

	/// <summary>
	/// ゲームオーバー中に、再開の入力があればシーンを終了する（Gameシーンが最初からやり直される）。
	/// </summary>
	void UpdateGameOver();

	/// <summary>
	/// 表示用の値をまとめてGameHudへ渡し、強化の選択中は画面のUI（HUDと連鎖数）を隠す。GameScene::Updateの最後に呼ぶ。
	/// </summary>
	void UpdateHud();

	/// <summary>
	/// ライトの調整値を反映する。
	/// </summary>
	void UpdateLight();
};

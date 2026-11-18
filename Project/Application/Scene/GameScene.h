#pragma once
#include "IScene.h"

// エンジン機能をインクルード
#include "Application/Game/Camera/GameCamera.h"
#include "DebugParameter.h"

#include "Application/Scene/Transition/Fade.h"
#include "Application/Game/Player/PlayerStats.h"
#include "Application/Game/Player/Player.h"
#include "Application/Game/Field/Field.h"
#include "Application/Game/Field/FieldScenery.h"
#include "Application/Game/Enemy/EnemyManager.h"
#include "Application/Game/Effect/ShatterEffectManager.h"
#include "Application/Game/Effect/IceMistEffect.h"
#include "Application/Game/IceShard/IceShardManager.h"
#include "Application/Game/Player/PadVibration.h"
#include "Application/Game/Phase/PhaseDirector.h"
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
	std::string NextSceneName() override { return nextSceneName_; }

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
	GameEngine::Model* sphereModel_ = nullptr;   // 氷塊の仮の見た目
	GameEngine::Model* planeModel_ = nullptr;    // 氷霧のパーティクルの板
	GameEngine::Model* shellShardModel_ = nullptr;  // 氷の殻の破片（球の殻を12個に割ったモデル）
	GameEngine::Model* pillarModel_ = nullptr;      // 氷柱（上が細い六角柱）

	// 背景のテクスチャハンドル
	uint32_t skyboxTextureHandle_ = 0;

	// 氷ゲージの矩形に使う白いテクスチャのハンドル
	uint32_t whiteTextureHandle_ = 0;
	uint32_t comboBurstTextureHandle_ = 0; // HIT表示の氷の破裂の画像

	// HPアイコン（3D）のモデル。枠・核・欠片
	HpHudModels hpHudModels_;
	PlayerViewAssets playerViewAssets_; // 自機の核と枠のモデル（hpCore.gltf、hpFrame.gltf）
	IceAreaAssets iceAreaAssets_;   // 氷エリアの描画に使うモデルとテクスチャ

private: // ゲームの状態

	// プレイヤーの固定の値（Playerが参照する）。移動速度は調整値として保存する
	PlayerStats playerStats_;

	// GameObjectManagerが所有するオブジェクトへの非所有ポインタ。シーン終了まで有効
	Player* player_ = nullptr;
	Field* field_ = nullptr;
	EnemyManager* enemyManager_ = nullptr;
	IceShardManager* iceShardManager_ = nullptr;
	PhaseDirector* phaseDirector_ = nullptr;
	GameHud* gameHud_ = nullptr;
	ComboPopupView* comboPopupView_ = nullptr;
	GameOverView* gameOverView_ = nullptr;

	// ゲームが終わって（ゲームオーバーかクリア）、終わりの表示中か。Initializeで戻す
	bool isGameEnded_ = false;

	// シーンを終えたあとに進む先のシーン名。終わりの表示で選ばれた行き先で決まる
	std::string nextSceneName_ = kGameSceneName;

	// コントローラーの振動。シーンが所有し、Initializeのたびに作り直す（破棄時に振動を止める）
	std::unique_ptr<PadVibration> padVibration_;
	ShatterEffectManager* shatterEffect_ = nullptr;
	IceMistEffect* iceMistEffect_ = nullptr;

private: // 定数

	// 破片モデルの名前（拡張子つき。モデルはファイル名で取得する）
	inline static const char* kShellShardModelName = "IceShellShard12.gltf";
	inline static const char* kPillarModelName = "IcePillar";
	// 氷柱の形（六角柱。上が細い）
	inline static constexpr uint32_t kPillarDivide = 6;
	inline static constexpr float kPillarTopRadius = 0.6f;
	inline static constexpr float kPillarBottomRadius = 1.0f;
	inline static constexpr float kPillarHeight = 1.0f;

	// 遷移先のシーン名（SetUpScenes.hの登録名）
	inline static const char* kGameSceneName = "Game";
	inline static const char* kTitleSceneName = "Title";

private: // 調整値

	// ライト
	float lightIntensity_ = 0.9f;
	Vector3 lightDirection_ = { -0.3f, -1.0f, 0.4f };
	Vector4 lightColor_ = { 0.85f, 0.92f, 1.0f, 1.0f }; // 夜の冷たい光

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
	/// <param name="sphereModel">氷塊に使うモデル</param>
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
	/// 現在のフェーズの出現ルールをEnemyManagerへ渡す。
	/// </summary>
	void ApplySpawnRule();

	/// <summary>
	/// ゲームの世界を止めているかを返す。ゲームが終わって終わりの表示中。
	/// </summary>
	bool IsGameStopped() const;

	/// <summary>
	/// ゲームのタイムスケールを決める。FpsCounter::SetTimeScaleを呼ぶのはこの関数だけにする。
	/// 止めているとき（IsGameStopped）は0、それ以外は1。
	/// </summary>
	void UpdateTimeScale();

	/// <summary>
	/// ゲームの終わりを判定する。ルールで変わる終わりの条件はこの関数だけに置く。
	/// HPが0ならゲームオーバー、デバッグキー（F8）ならクリア（ボスができるまでの仮）。
	/// </summary>
	void CheckGameEnd();

	/// <summary>
	/// ゲームを終える。終わりの状態を立て、入力と振動を止め、終わりの表示を開く。
	/// </summary>
	/// <param name="endType">ゲームの終わり方</param>
	void EndGame(GameEndType endType);

	/// <summary>
	/// 終わりの表示で行き先が選ばれたら、遷移先を決めてシーンを終了する（やり直すはGame、タイトルへはTitle）。
	/// </summary>
	void UpdateGameEnd();

	/// <summary>
	/// 表示用の値をまとめてGameHudへ渡す。GameScene::Updateの最後に呼ぶ。
	/// </summary>
	void UpdateHud();

	/// <summary>
	/// ライトの調整値を反映する。
	/// </summary>
	void UpdateLight();
};

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
	std::string NextSceneName() override { return "Title"; }

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

	// 背景のテクスチャハンドル
	uint32_t skyboxTextureHandle_ = 0;

	// 氷ゲージの矩形に使う白いテクスチャのハンドル
	uint32_t whiteTextureHandle_ = 0;

private: // ゲームの状態

	// プレイヤーの強化値（Playerが参照する）
	PlayerStats playerStats_;

	// GameObjectManagerが所有するオブジェクトへの非所有ポインタ。シーン終了まで有効
	Player* player_ = nullptr;
	Field* field_ = nullptr;
	EnemyManager* enemyManager_ = nullptr;

private: // 調整値

	// ライト
	float lightIntensity_ = 0.9f;
	Vector3 lightDirection_ = { -0.3f, -1.0f, 0.4f };

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
	/// ライトの調整値を反映する。
	/// </summary>
	void UpdateLight();
};

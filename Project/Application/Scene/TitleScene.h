#pragma once
#include "IScene.h"

// エンジン機能をインクルード
#include "Camera.h"

#include "Application/Scene/Transition/Fade.h"
#include "Application/Game/UI/TitleView.h"

/// <summary>
/// タイトルのシーン。決定（Spaceとパッドの A）でGameシーンへ進む。
/// </summary>
class TitleScene : public GameEngine::IScene {
public:
	TitleScene();

	/// <summary>
	/// タイトルの間だけ切ったフォグを、元の有効・無効へ戻す。
	/// </summary>
	~TitleScene();

	/// <summary>
	/// 初期化。デバッグのリセットでも呼ばれるため、前回のオブジェクトを先に削除する。
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 更新処理。決定の入力があればシーンを終了する。
	/// </summary>
	void Update() override;

	/// <summary>
	/// デバック時、処理して良いものを更新する
	/// </summary>
	void DebugUpdate() override {}

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw() override {}

	/// <summary>
	/// 次のシーン遷移する場面の名前を取得
	/// </summary>
	/// <returns></returns>
	std::string NextSceneName() override { return "Game"; }

	/// <summary>
	/// 遷移する演出
	/// </summary>
	/// <returns></returns>
	std::unique_ptr<ITransitionEffect> GetTransitionEffect() override { return std::make_unique<Fade>(); }

private: // シーン機能

	// 描画に必要な固定のカメラ（表示は2Dだけだが、描画はカメラの設定を前提にする）
	std::unique_ptr<GameEngine::Camera> camera_;

private: // アセット

	// 背景の板に使う白いテクスチャのハンドル
	uint32_t whiteTextureHandle_ = 0;

private: // ゲームの状態

	// GameObjectManagerが所有するオブジェクトへの非所有ポインタ。シーン終了まで有効
	TitleView* titleView_ = nullptr;

	// タイトルに入る前のフォグの有効・無効。デストラクタで戻す
	bool wasFogActive_ = false;

private: // 定数

	// ポストエフェクトのグラフにあるフォグのノード名（PostEffectGraph.jsonのname）
	inline static const char* kFogEffectName = "Fog";

private:

	/// <summary>
	/// 入力のコマンドを設定する
	/// </summary>
	void InputRegisterCommand();

	/// <summary>
	/// テクスチャを取得する。最後に取得できたかを確認する。
	/// </summary>
	void LoadAssets();

	/// <summary>
	/// 描画に使うカメラを設定する。
	/// </summary>
	void SetUpEnvironment();

	/// <summary>
	/// タイトルの間だけフォグを切る。元の有効・無効を覚えておく。
	/// フォグはレイトレーシングの深度を読むが、3Dの物体がないとレイトレーシングが飛ばされ、
	/// 深度の画像が書き込み用の状態のまま読まれてD3D12の検証エラーになるため。
	/// </summary>
	void DisableFog();

	/// <summary>
	/// DisableFogで切ったフォグを元の有効・無効へ戻す。
	/// </summary>
	void RestoreFog();
};

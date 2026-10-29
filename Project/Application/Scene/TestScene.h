#pragma once
#include"IScene.h"

// エンジン機能をインクルード
#include "Camera.h"
#include "Model.h"
#include "WorldTransform.h"
#include "Animator.h"
#include "ParticleBehavior.h"
#include "Material.h"
#include "RefBuffer.h"
#include "IceMaterial.h"
#include "Collider.h"
#include "DestructibleObject.h"
#include "Text.h"

#include "Application/Scene/Transition/Fade.h"

class TestScene : public GameEngine::IScene {
public:
	TestScene();
	~TestScene();

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
	void DebugUpdate() override;

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw() override;

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

	// メインカメラ
	std::unique_ptr<GameEngine::Camera> mainCamera_;

	GameEngine::Model* model_;
	GameEngine::WorldTransform world_;
	// アニメーションデータ
	std::map<std::string, AnimationData> walkAnimationData_;
	// アニメーションを再生するクラス
	std::unique_ptr<GameEngine::Animator> walkAnimator_;

	// ライト
	float intensity_ = 0.8f;
	Vector3 dir_ = { 0.0f,-1.0f,0.0f };
	Vector4 lightColor_ = { 1.0f,1.0f,1.0f,1.0f };

	// テキストのテスト
	std::unique_ptr<GameEngine::Text> text_;

	// 地面
	GameEngine::Model* terrainModel_;
	GameEngine::WorldTransform terrainWorld_;
};
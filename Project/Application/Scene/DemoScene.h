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

class DemoScene : public GameEngine::IScene {
public:
	DemoScene();
	~DemoScene();

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

	// パラメータ機能
	GameEngine::DebugParameter debugParame_{ "DemoScene" };

	// メインカメラ
	std::unique_ptr<GameEngine::Camera> mainCamera_;

	// プリミティブのエフェクト
	GameEngine::ParticleBehavior* testEffect_;
	
	float intensity_ = 0.8f;
	Vector3 dir_ = { 0.0f,-1.0f,0.0f };
	Vector4 lightColor_ = { 1.0f,1.0f,1.0f,1.0f };
	
	// テキストのテスト
	std::unique_ptr<GameEngine::Text> text_;
	std::string textName_;
	static inline char buf[128] = "";

	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
	float roughness_ = 0.5f;
	// 屈折
	float ior_ = 1.31f;

	float timer_ = 0.0f;
	float maxTime_ = 1.0f;
	GameEngine::EaseCurve easeCurve_;
};

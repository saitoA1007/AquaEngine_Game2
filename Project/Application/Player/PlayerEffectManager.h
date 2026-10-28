#pragma once
#include "IGameObject.h"
#include "GameObjectManager.h"
#include "ParticleBehavior.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Effect/PlayerHitAttackEffect.h"
#include "EffectsManager.h"

class PlayerEffectManager : public GameEngine::IGameObject {
public:
	PlayerEffectManager(GameEngine::GameObjectManager* objectManager, GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager,
		GameEngine::EffectsManager* effectsManager);
	~PlayerEffectManager() = default;

	// 初期化処理
	//void Initialize() override;
	//
	//// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	void StartShockWave(Vector3 pos);

	// ヒットエフェクト
	void StartHitEffect(Vector3 pos, uint32_t level);

	// 着地エフェクト
	void StartLandingEffect(Vector3 pos);

	// 落下エフェクト
	void StartDownAttackEffect(Vector3 pos, bool isActive);

	// チャージエフェクト
	void StartChargeEffect(Vector3 pos, Vector4 color, bool isActive);

private:
	GameEngine::GameObjectManager* objectManager_ = nullptr;
	GameEngine::EffectsManager* effectsManager_ = nullptr;

	// 攻撃がヒットした時
	std::vector<std::unique_ptr<GameEngine::EffectObject>> playerHitAttackEffects_;

	// 落下エフェクト
	std::unique_ptr<GameEngine::EffectObject> downAttackEffect_;

	// チャージエフェクト
	GameEngine::ParticleBehavior* chargeEffect_;
	// チャージエフェクトの親行列
	Matrix4x4 chargeParentMatrix_{};

	GameEngine::Model* shockModel_ = nullptr;
	GameEngine::Model* planeXZmodel_ = nullptr;
	GameEngine::Model* planeXYmodel_ = nullptr;

	uint32_t shockGH_ = 0;
	uint32_t dissolveNoiseGH_ = 0;

	uint32_t blastGH_ = 0;

	uint32_t crackGH_ = 0;
	uint32_t dissolveCrackGH_ = 0;

	GameEngine::ParticleBehavior* blastEffect_ = nullptr;
	GameEngine::ParticleBehavior* afterEffect_ = nullptr;

	float timer_ = 1.0f;
	float afterTimer_ = 1.0f;

	// ヒット演出
	PlayerHitAttackEffect* playerHitAttackEffect_ = nullptr;

	// 着地パーティクル
	GameEngine::ParticleBehavior* landingEffect_ = nullptr;
};
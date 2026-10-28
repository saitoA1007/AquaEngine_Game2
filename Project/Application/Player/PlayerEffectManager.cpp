#include "pch.h"
#include "PlayerEffectManager.h"
#include "Effect/ShockWave.h"
#include "Effect/ShockFloor.h"
#include "FPSCounter.h"
#include "MyMath.h"
using namespace GameEngine;

PlayerEffectManager::PlayerEffectManager(GameEngine::GameObjectManager* objectManager, GameEngine::ModelManager* modelManager,
	GameEngine::TextureManager* textureManager, GameEngine::EffectsManager* effectsManager) {

	// エフェクト管理機能を取得
	effectsManager_ = effectsManager;

	// プレイヤーがボスに攻撃を当てた時のエフェクト
	playerHitAttackEffects_.reserve(2);
	for (int i = 0; i < 2; ++i) {
		std::unique_ptr<EffectObject> effectObject = effectsManager_->GetEffect("BossHitEffect");
		if (effectObject) {
			playerHitAttackEffects_.push_back(std::move(effectObject));
		}
	}

	// 落下エフェクト
	downAttackEffect_ = effectsManager_->GetEffect("PlayerDownAttackEffect");

	auto* shockWaveModel = modelManager->GetNameByModel("RushPower.obj");
	shockWaveModel->SetDefaultIsEnableLight(false);
	auto* planeXZModel = modelManager->GetNameByModel("planeXZ.obj");
	planeXZModel->SetDefaultIsEnableLight(false);
	auto* planeXYmodel = modelManager->GetNameByModel("plane.obj");
	planeXYmodel->SetDefaultIsEnableLight(false);

	auto* waveModel = modelManager->GetNameByModel("rushWave.obj");
	waveModel->SetDefaultIsEnableLight(false);

	objectManager_ = objectManager;

	shockModel_ = shockWaveModel;
	planeXZmodel_ = planeXZModel;
	planeXYmodel_ = planeXYmodel;

	crackGH_ = textureManager->GetHandleByName("FX01_Crack_01.png");
	dissolveNoiseGH_ = textureManager->GetHandleByName("noise0.png");
	dissolveCrackGH_ = textureManager->GetHandleByName("FX01_Crack_01_crunch.png");
	shockGH_ = textureManager->GetHandleByName("Power.png");
	blastGH_ = textureManager->GetHandleByName("FX01_Flare_03.png");

	blastEffect_ = objectManager_->AddObject<ParticleBehavior>("HitEffect", 16, textureManager, planeXYmodel);
	blastEffect_->SetIsLoop(false);

	afterEffect_ = objectManager_->AddObject<ParticleBehavior>("HitAfterEffect", 32, textureManager, planeXYmodel);
	afterEffect_->SetIsLoop(false);

	// 着地エフェクト
	landingEffect_ = objectManager_->AddObject<ParticleBehavior>("PlayerLandingEffect", 32, textureManager, waveModel);
	landingEffect_->SetIsLoop(false);

	// プレイヤーのヒットエフェクト
	uint32_t hitEffectGH = textureManager->GetHandleByName("HitEffect.png");
	playerHitAttackEffect_ = objectManager_->AddObject<PlayerHitAttackEffect>(hitEffectGH, planeXYmodel);
	playerHitAttackEffect_->SetActive(false);

	// チャージ演出
	chargeEffect_ = objectManager_->AddObject<ParticleBehavior>("PlayerChargeParticle", 32, textureManager, waveModel);
	chargeEffect_->SetIsLoop(false);
	// 発生位置は親からの相対位置にする
	chargeParentMatrix_ = Matrix4x4::MakeTranslateMatrix({ 0.0f,0.0f,0.0f });
	chargeEffect_->SetEmitterPos({ 0.0f,0.0f,0.0f });
	chargeEffect_->SetParent(&chargeParentMatrix_, ParticleSimulationSpace::kLocal);
}

void PlayerEffectManager::Update() {

	if (timer_ <= 1.0f) {
		timer_ += FpsCounter::deltaTime / 0.3f;
		if (timer_ >= 1.0f) {
			landingEffect_->SetIsLoop(false);
			if (blastEffect_->IsLoop()) {
				blastEffect_->SetIsLoop(false);
			}
		}
	}

	if (afterTimer_ <= 1.0f) {
		afterTimer_ += FpsCounter::deltaTime / 0.8f;
		if (afterTimer_ >= 1.0f) {
			if (afterEffect_->IsLoop()) {
				afterEffect_->SetIsLoop(false);
			}
		}
	}

	// ヒットエフェクトの更新
	for (auto& effect : playerHitAttackEffects_) {
		effect->Update();
	}

	// 落下エフェクトの更新
	downAttackEffect_->Update();
}

void PlayerEffectManager::Draw() {
	// ヒットエフェクトの描画
	for (auto& effect : playerHitAttackEffects_) {
		effect->Draw();
	}

	// 落下エフェクトの描画
	downAttackEffect_->Draw();
}

void PlayerEffectManager::StartShockWave(Vector3 pos) {

	return;

	// 描画
	objectManager_->AddObject<ShockWave>(shockModel_, planeXYmodel_, blastGH_, shockGH_, dissolveNoiseGH_, pos);

	objectManager_->AddObject<ShockFloor>(planeXZmodel_, crackGH_, dissolveCrackGH_, pos);

	blastEffect_->SetEmitterPos(pos);
	blastEffect_->SetAttractionTarget(pos);
	blastEffect_->SetIsLoop(true);
	timer_ = 0.0f;

	afterEffect_->SetEmitterPos(pos);
	afterEffect_->SetAttractionTarget(pos);
	afterEffect_->SetIsLoop(true);
	afterTimer_ = 0.0f;
}


void PlayerEffectManager::StartHitEffect(Vector3 pos, uint32_t level) {
	playerHitAttackEffect_->Start(pos, level);

	if (playerHitAttackEffects_.empty()) { return; }
	for (auto& effect : playerHitAttackEffects_) {
		if (!effect->IsPlaying()) {
			effect->Play(pos);
			break;
		}
	}
}

void PlayerEffectManager::StartLandingEffect(Vector3 pos) {
	timer_ = 0.0f;
	landingEffect_->SetEmitterPos(pos);
	landingEffect_->SetIsLoop(true);
}

void PlayerEffectManager::StartDownAttackEffect(Vector3 pos, bool isActive) {

	if (isActive) {
		if (!downAttackEffect_->IsPlaying()) {
			downAttackEffect_->Play(pos);
		} else {
			downAttackEffect_->SetPosition(pos);
		}
	} else {
		downAttackEffect_->Stop();
	}
}

void PlayerEffectManager::StartChargeEffect(Vector3 pos, Vector4 color, bool isActive) {
	// 停止時は親を動かさず、残っているパーティクルはその場で消えるまで表示する
	if (isActive) {
		chargeParentMatrix_ = Matrix4x4::MakeTranslateMatrix(pos);
		chargeEffect_->SetColor(color);
	}
	chargeEffect_->SetIsLoop(isActive);
}
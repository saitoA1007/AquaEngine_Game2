#include "pch.h"
#include "WindAttack.h"
#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
#include "EasingManager.h"
#include "MyMath.h"
using namespace GameEngine;

WindAttack::WindAttack(GameEngine::Model* model, GameEngine::ParticleBehavior* windParticle) : modelComponent_(model) {

	modelComponent_.worldTransform_.transform_.translate = Vector3(0.0f,-10.0f,0.0f);
	windParticle_ = windParticle;

	// 風パーティクルを開始位置・方向に合わせて発生させる
	if (windParticle_ != nullptr) {
		windParticle_->SetIsLoop(false);
		windParticle_->SetEmitterPos(modelComponent_.worldTransform_.transform_.translate);
	}

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("WindAttack");
	debugParame_->Register("ColliderRadius", colliderSize_);
	debugParame_->Apply();

	// 当たり判定
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.SetAnchor(colliderAnchor_);
	collider_.UpdateOrientationsFromRotate(modelComponent_.worldTransform_.transform_.rotate);
	collider_.SetCollisionAttribute(kCollisionAttributeEnemy);
	collider_.SetCollisionMask(~kCollisionAttributeEnemy);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kWind);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック関数に登録する
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
		});
}

void WindAttack::Initialize() {
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.SetAnchor(colliderAnchor_);
	collider_.UpdateOrientationsFromRotate(modelComponent_.worldTransform_.transform_.rotate);
	modelComponent_.Update();
}

void WindAttack::Update() {
	debugParame_->ApplyIfDirty();

	timer_ += FpsCounter::gameDeltaTime / maxTime_;

	// 回転
	Vector3 dir = Slerp(startDir_, endDir_, timer_);

	// 方向から角度を取得
	modelComponent_.worldTransform_.transform_.rotate = Math::DirectionToEuler(dir);

	// 風パーティクルの位置と向きを追従させる
	if (windParticle_ != nullptr) {
		windParticle_->SetEmitterPos(modelComponent_.worldTransform_.transform_.translate);
		windParticle_->SetDirection(dir);
	}

	if (timer_ >= 1.0f) {

		isActive_ = false;
		// 当たり判定を無効
		collider_.SetActive(false);

		// 攻撃終了に合わせてパーティクルの発生を止める
		if (windParticle_ != nullptr) {
			windParticle_->SetIsLoop(false);
		}
	}

	modelComponent_.Update();
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.UpdateOrientationsFromRotate(modelComponent_.worldTransform_.transform_.rotate);
}

void WindAttack::Draw() {
	// 描画
	//modelComponent_.DrawRaytracing(renderQueue_);
}

void WindAttack::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

}
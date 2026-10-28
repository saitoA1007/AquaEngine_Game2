#include "pch.h"
#define NOMINMAX
#include <algorithm>
#include "Player.h"
#include "LogManager.h"
#include "MyMath.h"
#include "FPSCounter.h"
#include "Model.h"
#include "Application/CollisionConfig.h"
#include "Application/Enemy/RangedAttack/IceFall.h"
#include "Application/Enemy/BossEnemy.h"
using namespace GameEngine;

Player::Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model, GameEngine::AnimationManager* animationManager, PlayerEffectManager* effectManager) {
	inputCommand_ = inputCommand;
	model_ = model;

	// ワールド行列を初期化
	worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,2.0f,0.0f},{0.0f,2.0f,10.0f} });

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Player");
	// 値を登録
	moveAction_.RegisterParameter(debugParame_.get());
	bounceAction_.RegisterParameter(debugParame_.get());
	attackRushAction_.RegisterParameter(debugParame_.get());
	playerAttackDownAction_.RegisterParameter(debugParame_.get());
	playerPhysics_.RegisterParameter(debugParame_.get());
	playerStatus_.RegisterParameter(debugParame_.get());
	debugParame_->Apply();

	// 当たり判定の設定
	collider_.SetRadius(1.0f);
	collider_.SetWorldPosition(worldTransform_.transform_.translate);
	collider_.SetCollisionAttribute(kCollisionAttributePlayer);
	collider_.SetCollisionMask(~kCollisionAttributePlayer);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kPlayer);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック登録
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionStay(result);
		});

	// アニメーション管理
	animator_ = std::make_unique<PlayerAnimator>(model, animationManager);
	commonData_.animator_ = animator_.get();
	// 演出管理
	commonData_.effectManager_ = effectManager;
	effectManager_ = effectManager;

	// 移動アクション
	moveAction_.Initialize(&commonData_, inputCommand);
	// 跳ね返りアクション
	bounceAction_.Initialize(&commonData_);
	// 突進アクション
	attackRushAction_.Initialize(&commonData_, inputCommand);
	// 落下攻撃アクション
	playerAttackDownAction_.Initialize(&commonData_, inputCommand);

	// 重力
	playerPhysics_.Initialize(&commonData_);

	// プレイヤーの状態
	playerStatus_.Initialize(&commonData_);
}

void Player::Initialize() {
	// ステータスを初期化
	commonData_ = PlayerCommonData{};
	commonData_.animator_ = animator_.get();
	commonData_.effectManager_ = effectManager_;
	commonData_.currentYaw = std::atan2f(commonData_.currentDir.x, commonData_.currentDir.z);

	// 位置を初期化
	worldTransform_.transform_.translate = { 0.0f,2.0f,12.0f };
	collider_.SetWorldPosition(worldTransform_.transform_.translate);

	isDraw_ = true;

	// 体力をリセット
	playerStatus_.ResetHp();

	// 初期化
	animator_->Initialize();
}

void Player::Update() {
	// 値の適応
	debugParame_->ApplyIfDirty();

	// カメベクトルを更新
	moveAction_.UpdateCameraBasis(camera_->GetWorldMatrix());

	// 移動操作
	moveAction_.ProcessInput();
	// 突進操作
	attackRushAction_.ProcessInput();
	// 落下攻撃操作
	playerAttackDownAction_.ProcessInput();

	// 落下攻撃の更新処理
	playerAttackDownAction_.Update();
	// 受ける物理を更新
	playerPhysics_.Update();
	// 更新
	attackRushAction_.Update();
	// 状態
	playerStatus_.Update();

	// 範囲制限
	ApplyClamp();

	// 移動を適応
	worldTransform_.transform_.translate += commonData_.velocity * FpsCounter::gameDeltaTime;

	// 角度を更新
	UpdateRotation();

	// 行列の更新
	worldTransform_.UpdateTransformMatrix();
	// 当たり判定の更新
	collider_.SetWorldPosition(worldTransform_.GetWorldPosition());

	// 更新
	commonData_.transform = worldTransform_.transform_;

	// アニメーションの更新
	animator_->Update();
}

void Player::Draw() {
	if (!isDraw_) { return; }

	// モデル描画
	renderQueue_->SubmitRaytracingModel(model_, worldTransform_);
}

void Player::UpdateRotation() {
	// 現在向いている角度を更新
	commonData_.currentDir = Vector3(commonData_.velocity.x, 0.0f, commonData_.velocity.z);
	commonData_.currentDir.Normalize();

	// 方向を取得
	Vector3 targetDir = { 0.0f, 0.0f, 0.0f };
	if (commonData_.targetDir.x != 0.0f || commonData_.targetDir.z != 0.0f) {
		targetDir = commonData_.targetDir;
	} else {
		targetDir = commonData_.currentDir;
	}

	// 回転の更新
	if (targetDir.x != 0.0f || targetDir.z != 0.0f) {
		targetDir.y = 0.0f;
		targetDir.Normalize();

		// 目標ヨー角を計算
		float targetYaw = std::atan2f(targetDir.x, targetDir.z);

		// 最短経路で角度補間をして現在の角度を求める
		float kRotationLerpSpeed_ = 10.0f;
		float maxStep = kRotationLerpSpeed_ * FpsCounter::gameDeltaTime;
		commonData_.currentYaw = Math::LerpShortAngle(commonData_.currentYaw, targetYaw, maxStep);

		// ヨー角をワールドトランスフォームに反映
		worldTransform_.transform_.rotate.y = commonData_.currentYaw;
		// 現在の角度を更新
		commonData_.currentDir = Math::YawToDirection(commonData_.currentYaw);
	}
}

void Player::ApplyClamp() {
	if (commonData_.state == PlayerState::kAttackDown) {
		commonData_.velocity.y = std::max(commonData_.velocity.y, -playerAttackDownAction_.GetAttackDownMaxSpeed());
	} else {
		commonData_.velocity.y = std::max(commonData_.velocity.y, -playerPhysics_.GetMaxFallSpeed());
	}
}

void Player::OnCollisionStay(const GameEngine::CollisionResult& result) {

	bool isWall = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kWall));
	bool isBoss = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kBoss));
	bool isFloor = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kGround));
	bool isIceFall = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kIceFall));
	bool isWind = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kWind));

	// 床の衝突
	if (isFloor) {
		worldTransform_.transform_.translate.y += std::fabs(result.contactNormal.y) * result.penetrationDepth;
		// 地面にめり込んだ時元に戻す
		if (worldTransform_.transform_.translate.y < 0.0f) {
			worldTransform_.transform_.translate.y = 0.0f;
		}
	
		if (commonData_.velocity.y <= 0.0f) {

			// 速度を0にする
			commonData_.velocity.y = 0.0f;

			// 空中浮遊状態なら歩き状態へ
			if (commonData_.animator_->GetCurrentType() == PlayerAnimationType::kAirMove) {
				commonData_.animator_->StartAnimation(PlayerAnimationType::kWalk, "歩き");
			}

			if (commonData_.state == PlayerState::kAttackDown) {
				// 地面破壊スタート
				commonData_.effectManager_->StartShockWave(Vector3(worldTransform_.transform_.translate.x, 1.0f, worldTransform_.transform_.translate.z));

				// 着地エフェクトを開始
				effectManager_->StartLandingEffect(Vector3(worldTransform_.transform_.translate.x, 0.0f, worldTransform_.transform_.translate.z));

				commonData_.state = PlayerState::kStiffness;
				commonData_.animator_->StartAnimation(PlayerAnimationType::kWalk, "歩き");
				Log("Player End attackDown");
			}

			// 地面に接触状態
			if (commonData_.state == PlayerState::kJump) {
				commonData_.state = PlayerState::kNone;
				commonData_.animator_->StartAnimation(PlayerAnimationType::kWalk, "歩き");
			}
		}	
	}

	// 壁の衝突処理
	if (isWall) {
		bounceAction_.WallBounce(worldTransform_.transform_.translate, result.contactNormal, result.penetrationDepth, attackRushAction_.GetRushMaxSpeed());
	}

	// 氷柱との衝突
	if (isIceFall) {
		IceFall* iceFall = result.userData.As<IceFall>();
		if (iceFall == nullptr) { return; }

		// 氷柱を削除
		if (commonData_.state == PlayerState::kAttackRush) {
			iceFall->Destroy();
			//iceFall->SetIsBreak(true);
		}

		bounceAction_.WallBounce(worldTransform_.transform_.translate, result.contactNormal * -1.0f, result.penetrationDepth, attackRushAction_.GetRushMaxSpeed());
	}

	// 風による攻撃
	if (isWind) {
		// ダメージを受ける
		playerStatus_.TakeDamage(1);
	}

	// ボスの衝突判定
	if (isBoss) {

		BossEnemy* boss = result.userData.As<BossEnemy>();
		if (boss == nullptr) { return; }

		if (boss->GetBossState() == BossState::kIn) {
			// 跳ね返る
			bounceAction_.WallBounce(worldTransform_.transform_.translate, result.contactNormal * -1.0f, result.penetrationDepth, attackRushAction_.GetRushMaxSpeed());
		} else {

			if (commonData_.state != PlayerState::kAttackDown) {

				// ダメージを受ける
				playerStatus_.TakeDamage(1);
			}
		}
	}
}

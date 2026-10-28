#include "pch.h"
#include "BossEnemy.h"

// 各状態
#include "State/BossStateIn.h"
#include "State/BossStateBattle.h"
#include "State/BossStateOut.h"

#include "Application/CollisionConfig.h"
#include "Application/Player/Player.h"

using namespace GameEngine;

BossEnemy::BossEnemy(GameEngine::Model* model, GameEngine::Model* eggModel, GameEngine::WorldTransform& playerWorld, GameEngine::AnimationManager* animationManager, BossRangedAttackManager* rangedAttackManager)
	: modelComponent_(model) {

	// 卵モデルを取得
	eggModel_ = eggModel;

	// 初期化
	modelComponent_.worldTransform_.Initialize({ {3.0f,3.0f,3.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// 参照するマテリアルを変更
	uint32_t meshNum = model->GetMeshes().size();
	iceMaterial_.resize(meshNum);
	for (uint32_t i = 0; i < meshNum; ++i) {
		iceMaterial_[i].materialData_->dissolveThreshold = 0.0f;
		iceMaterial_[i].materialData_->color = {0.0f,0.2f,0.8f,1.0f};
		modelComponent_.SetBufferMaterial(0, iceMaterial_[i].GetMaterialSrvIndex(), i);
		modelComponent_.SetHitGroup(1, i);
	}

	// アニメーション
	animator_ = std::make_unique<BossAnimator>(model, animationManager);

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("BossEnemy");
	debugParame_->Register("MaxHp", maxHp_);
	debugParame_->Register("Scale", modelComponent_.worldTransform_.transform_.scale);
	debugParame_->Register("ColliderRadius", colliderRadius_,0,"Collider");
	debugParame_->Register("ColliderOffsetPosY", colliderOffsetPosY_, 1,"Collider");

	for (uint32_t i = 0; i < meshNum; ++i) {
		std::string name = "Color" + std::to_string(i);
		debugParame_->Register(name, iceMaterial_[i].materialData_->color);
	}

	// 共通データ設定
	stateCommonData_.worldTransform = &modelComponent_.worldTransform_;
	stateCommonData_.animator = animator_.get();
	stateCommonData_.debugParame = debugParame_.get();

	// 状態の生成
	statesTable_[static_cast<size_t>(BossState::kIn)] = std::make_unique<BossStateIn>(stateCommonData_);
	statesTable_[static_cast<size_t>(BossState::kBattle)] = std::make_unique<BossStateBattle>(stateCommonData_, &playerWorld.transform_.translate, rangedAttackManager);
	statesTable_[static_cast<size_t>(BossState::kOut)] = std::make_unique<BossStateOut>(stateCommonData_);

	// 最初の状態を設定する
	bossState_ = BossState::kIn;
	currentState_ = statesTable_[static_cast<size_t>(BossState::kIn)].get();
	currentState_->Enter();

	// 当たり判定を設定
	collider_.SetRadius(colliderRadius_);
	collider_.SetWorldPosition(modelComponent_.worldTransform_.transform_.translate + Vector3(0.0f, colliderOffsetPosY_, 0.0f));
	collider_.SetCollisionAttribute(kCollisionAttributeEnemy);
	collider_.SetCollisionMask(~kCollisionAttributeEnemy);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kBoss);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック登録
	collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
	});

	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionStay(result);
	});

	debugParame_->Apply();
}

void BossEnemy::Initialize() {
	modelComponent_.worldTransform_.transform_.translate = { 0.0f,0.0f,0.0f };
	// アニメーション初期化
	animator_->Initialize();

	stateCommonData_.hp_ = maxHp_;

	stateCommonData_.isBreakEgg = false;
	stateCommonData_.isDrawEgg = true;

	// 初期化
	if (currentState_) {
		currentState_->Exit();
	}
	bossState_ = BossState::kIn;
	currentState_ = statesTable_[static_cast<size_t>(BossState::kIn)].get();
	currentState_->Enter();
}

void BossEnemy::Update() {
	// 値の適応
	debugParame_->ApplyIfDirty();

	// 状態変更が有効であれば、切り替える
	if (stateCommonData_.bossStateRequest) {
		currentState_->Exit();
		bossState_ = stateCommonData_.bossStateRequest.value();
		currentState_ = nullptr;
		currentState_ = statesTable_[static_cast<size_t>(*stateCommonData_.bossStateRequest)].get();
		currentState_->Enter();
		stateCommonData_.bossStateRequest = std::nullopt;
	}

	// 現在の状態の更新処理
	currentState_->Update();

	// 行列の更新
	modelComponent_.Update();

	// 当たり判定の位置を更新
	if (stateCommonData_.isBreakEgg) {
		collider_.SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition() + Vector3(0.0f, colliderOffsetPosY_, 0.0f));
		collider_.SetRadius(colliderRadius_);
	} else {
		collider_.SetWorldPosition(modelComponent_.worldTransform_.GetWorldPosition() + Vector3(0.0f, 0.0f, 0.0f));
		collider_.SetRadius(colliderRadius_);
	}

	// アニメーションの更新
	animator_->Update();
}

void BossEnemy::Draw() {

	if (stateCommonData_.isDrawEgg) {
		// 卵モデルを描画
		renderQueue_->SubmitRaytracingModel(eggModel_, modelComponent_.worldTransform_);
	} else {
		// 描画
		modelComponent_.DrawRaytracing(renderQueue_);
	}
}

void BossEnemy::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {


	bool isPlayer = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kPlayer));

	Player* player = nullptr;
	if (isPlayer) {
		// プレイヤーを取得
		player = result.userData.As<Player>();
		if (player == nullptr) { return; }
	}

	if (bossState_ == BossState::kIn) {

		// 卵の破壊フラグを有効
		if (player != nullptr) {
			if (player->GetCurrentState() == PlayerState::kAttackDown) {
				stateCommonData_.isBreakEgg = true;
			}
		}
	} else {

		if (player != nullptr) {
			// プレイヤーが攻撃状態の時にダメージを受ける
			if (player->GetCurrentState() == PlayerState::kAttackDown) {
				float damage = player->GetDamage();
				stateCommonData_.hp_ -= static_cast<int32_t>(damage);
				// ヒット時の演出
				player->StartHitEffect(stateCommonData_.worldTransform->transform_.translate + Vector3(0.0f,10.0f,0.0f));
			}
		}
	}
}

void BossEnemy::OnCollisionStay([[maybe_unused]] const GameEngine::CollisionResult& result) {


}

BossBattleState BossEnemy::GetBattleState() const {
	if (bossState_ != BossState::kBattle) { return BossBattleState::kWait; }

	// 現在の状態を取得
	BossStateBattle* battleState = dynamic_cast<BossStateBattle*>(statesTable_[static_cast<size_t>(bossState_)].get());
	return battleState->GetBattleState();
}
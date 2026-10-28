#include "pch.h"
#include "Wall.h"
#include "FPSCounter.h"
#include "DebugParameter.h"
#include "Application/CollisionConfig.h"
#include "Application/Player/Player.h"
#include "Application/Enemy/BossEnemy.h"
using namespace GameEngine;

Wall::Wall(GameEngine::Model* model, GameEngine::Model* fractureModel, GameEngine::DebugParameter* parame) : underWallModelComponent_(model),
	destructObject_("Wall", fractureModel, static_cast<uint32_t>(CollisionTypeID::kWall), kCollisionAttributeTerrain) {
	// パラメーター機能を取得
	parame_ = parame;

	std::string subGroup = "Wall";
	int index = 0;
	parame_->Register("ModelScale", destructObject_.worldTransform_.transform_.scale, index++, subGroup);
	parame_->Register("ColliderSize", colliderSize_, index++, subGroup);
	parame_->Register("MaxHp", maxHp_, index++, subGroup);
	parame_->Register("RespawnTime", respawnTime_, index++, subGroup);
	parame_->Register("ReassembleDestroyedRatio", reassembleDestroyedRatio_, index++, subGroup);

	// 破片側の当たり判定を無効
	destructObject_.SetIsColliderActive(false);

	// 当たり判定
	collider_.SetWorldPosition(destructObject_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.UpdateOrientationsFromRotate(destructObject_.worldTransform_.transform_.rotate);
	collider_.SetCollisionAttribute(kCollisionAttributeTerrain);
	collider_.SetCollisionMask(~kCollisionAttributeTerrain);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kWall);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック関数に登録する
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
	});

	underWallModelComponent_.SetBufferMaterial(0, iceMaterial_.GetMaterialSrvIndex());
	underWallModelComponent_.SetHitGroup(1);
}

void Wall::SetParameter(const Transform& transform) {
	// 位置を取得
	destructObject_.worldTransform_.transform_.translate = transform.translate;
	destructObject_.worldTransform_.transform_.rotate = transform.rotate;
	destructObject_.worldTransform_.transform_.scale = { 2.0f,2.0f,1.5f };

	// 下に存在する壁を設置する
	underWallModelComponent_.worldTransform_.transform_ = destructObject_.worldTransform_.transform_;
	underWallModelComponent_.worldTransform_.transform_.translate.y = -2.0f;
	underWallModelComponent_.worldTransform_.transform_.scale * 0.8f;

	// 初期化
	Initialize();
}

void Wall::Initialize() {
	collider_.SetWorldPosition(destructObject_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.UpdateOrientationsFromRotate(destructObject_.worldTransform_.transform_.rotate);
	destructObject_.Update();
	underWallModelComponent_.Update();
}

void Wall::Update() {

	// 破片の更新処理
	destructObject_.Update();

	// 設定した破壊率を超えたら、壁を元に戻す
	if (!destructObject_.IsReassembling()) {
		if (destructObject_.GetDestroyedRatio() >= reassembleDestroyedRatio_) {
			if (!isBreakIce_) {
				isBreakIce_ = true;
			}
		}
	}

	if (!isBreakIce_) { return; }
	respawnTimer_ += FpsCounter::gameDeltaTime / respawnTime_;

	// リスポーン時間を超えたら、復活する
	if (respawnTimer_ >= 1.0f) {
		isBreakIce_ = false;
		respawnTimer_ = 0.0f;
		//destructObject_.worldTransform_.transform_.scale.z = 1.5f;
		currentHp_ = maxHp_;

		// 破片を元に戻す
		destructObject_.Reassemble();
	}
}

void Wall::Draw() {

	// 下に存在する壁を設置する
	underWallModelComponent_.DrawRaytracing(renderQueue_);

	// 壁を描画
	//modelComponent_.DrawRaytracing(renderQueue_);

	// 破片を描画
	destructObject_.Draw();
}

void Wall::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

	if (isBreakIce_) { return; }

	bool isPlayer = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kPlayer));
	bool isBoss = (result.userData.typeID == static_cast<uint32_t>(CollisionTypeID::kBoss));

	Player* player = nullptr;
	BossEnemy* boss = nullptr;

	if (isPlayer) { 
		player = result.userData.As<Player>();
	}

	if (isBoss) {
		boss = result.userData.As<BossEnemy>();
	}

	// Hpを削る
	if (player != nullptr) {

		// 突進攻撃で壁にヒットした瞬間のみ（触れているだけでは反応しない）
		if (player->IsHitWall()) {
			player->SetIsHitWall(false);
			Vector3 velocity = player->GetVelocity();
			velocity.y = 0.0f;
			// 現在の速度の割合を取得
			float ratio = velocity.Length() / player->GetRushMaxSpeed();

			if (ratio <= 0.3f) {
				currentHp_ -= 1;
			} else if (ratio <= 0.7f) {
				currentHp_ -= 2;
			} else {
				currentHp_ -= 3;
			}

			// 攻撃を受けた位置に破片を飛び散らせる
			destructObject_.OnCollisionEnter(result);
		}
	} else if (boss != nullptr) {

		BossBattleState battleState = boss->GetBattleState();

		// ボスが突進状態であれば
		if (battleState == BossBattleState::kRushAttack) {
			// ボスの場合、固定ダメージ
			currentHp_ -= 2;

			// 攻撃を受けた位置に破片を飛び散らせる
			destructObject_.OnCollisionEnter(result);
		}
	}

	//// hpによって形を帰る
	//if (currentHp_ == 2) {
	//	destructObject_.worldTransform_.transform_.scale.z = 1.0f;
	//} else if (currentHp_ == 1) {
	//	destructObject_.worldTransform_.transform_.scale.z = 0.5f;
	//}else if (currentHp_ <= 0) {
	//	currentHp_ = 0;
	//	isBreakIce_ = true;
	//}

	//destructObject_.worldTransform_.UpdateTransformMatrix();
}
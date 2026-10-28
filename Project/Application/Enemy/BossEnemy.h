#pragma once
#include <array>
#include <vector>
#include "IGameObject.h"
#include "Collider.h"
#include "ModelComponent.h"
#include "IceMaterial.h"
#include "IBossState.h"
#include "Application/Enemy/BattleAction/IBossBattleAction.h"

#include "WorldTransforms.h"

// 前方宣言
namespace GameEngine {
	class GameObjectManager;
}
class BossRangedAttackManager;

class BossEnemy : public GameEngine::IGameObject {
public:
	BossEnemy(GameEngine::Model* model, GameEngine::Model* eggModel, GameEngine::WorldTransform& playerWorld, GameEngine::AnimationManager* animationManager, BossRangedAttackManager* rangedAttackManager);
	~BossEnemy() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	/// <summary>
	/// ワールド行列を取得
	/// </summary>
	/// <returns></returns>
	GameEngine::WorldTransform& GetWorldTransform() { return modelComponent_.worldTransform_; }

	// 現在のHpを取得
	int32_t GetCurrentHp() const { return stateCommonData_.hp_; }

	// 最大Hpを取得
	int32_t GetMaxHp() const { return maxHp_; }

	// 現在の戦闘中の状態を朱徳
	BossBattleState GetBattleState() const;

	// 現在の状態を取得
	BossState GetBossState() const {
		return bossState_;
	}

	// 卵が割れたか
	bool IsBreakEgg() const { return stateCommonData_.isBreakEgg; }

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// モデル
	GameEngine::ModelComponent modelComponent_;

	// 卵モデル
	GameEngine::Model* eggModel_;

	// 氷のマテリアル
	std::vector<GameEngine::IceMaterial> iceMaterial_;

	// 球の当たり判定
	GameEngine::SphereCollider collider_;

	// 状態
	std::array<std::unique_ptr<IBossState>, static_cast<size_t>(BossState::kMaxCount)> statesTable_;
	// 現在の状態
	IBossState* currentState_ = nullptr;
	BossState bossState_ = BossState::kIn;

	// 各状態で共有するデータ
	BossStateCommonData stateCommonData_;

	// ボスのアニメーション
	std::unique_ptr<BossAnimator> animator_;

	// 最大hp
	int32_t maxHp_ = 100;
	// 当たり判定
	float colliderRadius_ = 3.0f;
	// 当たり判定のオフセット
	float colliderOffsetPosY_ = 0.0f;

private:

	/// <summary>
	/// 当たり判定
	/// </summary>
	void OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result);

	void OnCollisionStay([[maybe_unused]] const GameEngine::CollisionResult& result);
};
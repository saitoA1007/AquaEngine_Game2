#pragma once
#include <unordered_map>
#include "Application/Enemy/IBossState.h"
#include "Application/Enemy/BattleAction/IBossBattleAction.h"

// 前方宣言
namespace GameEngine {
	class GameObjectManager;
}

class BossStateBattle : public IBossState {
public:
	// 行動に重みを付ける
	struct BehaviorWeight {
		BossBattleState behavior;
		uint32_t weight;
	};

public:
	BossStateBattle(BossStateCommonData& commonData, Vector3* playerPos, BossRangedAttackManager* rangedAttackManager);
	~BossStateBattle() = default;

	/// <summary>
	/// 入りの処理
	/// </summary>
	void Enter() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// 終わりの処理
	/// </summary>
	void Exit() override;

	// 現在の状態を取得
	BossBattleState GetBattleState() const { return currentBattleState_; }

private:
	BossStateCommonData& stateCommonData_;

	// 戦いの状態テーブル
	std::unordered_map<BossBattleState, std::unique_ptr<IBossBattleAction>> battleStatesTable_;
	BossBattleState currentBattleState_;

	BossBattleStateCommonData battleStateCommonData_;

	// 遷移するために使用するリスト
	std::vector<BehaviorWeight> lotteryList_;

private:

	/// <summary>
	/// 次の攻撃行動を取得する
	/// </summary>
	/// <returns></returns>
	BossBattleState SelectWeightedBattleState();
};
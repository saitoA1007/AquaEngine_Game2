#include "pch.h"
#include "BossStateBattle.h"
#include "RandomGenerator.h"
#include "LogManager.h"
#include "Application/Enemy/BattleAction/BossBattleAction.h"

using namespace GameEngine;

BossStateBattle::BossStateBattle(BossStateCommonData& commonData, Vector3* playerPos, BossRangedAttackManager* rangedAttackManager) :
	stateCommonData_(commonData) {

	// プレイヤーの位置を取得
	battleStateCommonData_.playerPos = playerPos;
	// アニメーション管理機能を取得
	battleStateCommonData_.animator = commonData.animator;
	battleStateCommonData_.rangedAttackManager = rangedAttackManager;
	battleStateCommonData_.transform = commonData.worldTransform->transform_;

	// 各行動を登録する
	battleStatesTable_[BossBattleState::kRushAttack] = std::make_unique<BossRushAttackAction>(battleStateCommonData_);
	battleStatesTable_[BossBattleState::kWait] = std::make_unique<BossWaitAction>(battleStateCommonData_);
	battleStatesTable_[BossBattleState::kRotateMove] = std::make_unique<RotateMoveAction>(battleStateCommonData_);
	battleStatesTable_[BossBattleState::kCrossMove] = std::make_unique<BossCrossMoveAction>(battleStateCommonData_);
	battleStatesTable_[BossBattleState::kIceFallAttack] = std::make_unique<IceFallAttackAction>(battleStateCommonData_);
	battleStatesTable_[BossBattleState::kWindAttack] = std::make_unique<WindAttackAction>(battleStateCommonData_);
	battleStatesTable_[BossBattleState::kResetMove] = std::make_unique<ResetAction>(battleStateCommonData_);

	// 初期化
	currentBattleState_ = BossBattleState::kResetMove;
	battleStatesTable_[currentBattleState_]->Initialize();

	// 行動に重み付け
	lotteryList_ = {
		{ BossBattleState::kRushAttack,10 },    // 突進攻撃
		{ BossBattleState::kWait,10 },          // 待機
		{ BossBattleState::kRotateMove,10 },    // ステージに沿った移動
		{ BossBattleState::kCrossMove,10 },     // 横断移動
		{ BossBattleState::kIceFallAttack,10 }, // 氷柱攻撃
		{ BossBattleState::kWindAttack,10 },    // 風攻撃
	};

	std::string subGroup = "StatusWeight";
	int index = 0;
	stateCommonData_.debugParame->Register("RushAttack", lotteryList_[0].weight, index++, subGroup);
	stateCommonData_.debugParame->Register("Wait", lotteryList_[1].weight, index++, subGroup);
	stateCommonData_.debugParame->Register("RotateMove", lotteryList_[2].weight, index++, subGroup);
	stateCommonData_.debugParame->Register("CrossMove", lotteryList_[3].weight, index++, subGroup);
	stateCommonData_.debugParame->Register("IceFallAttack", lotteryList_[4].weight, index++, subGroup);
	stateCommonData_.debugParame->Register("WindAttack", lotteryList_[5].weight, index++, subGroup);

	// 値を登録
	for (auto& [key,state] : battleStatesTable_) {
		state->RegisterParameter(stateCommonData_.debugParame);
	}
}

void BossStateBattle::Enter() {
	currentBattleState_ = BossBattleState::kResetMove;
	battleStatesTable_[currentBattleState_]->Initialize();
}

void BossStateBattle::Update() {

	// hpが0になれば撃破状態に遷移
	if (stateCommonData_.hp_ <= 0) {
		stateCommonData_.bossStateRequest = BossState::kOut;
	}

	// 切り替え処理
	if (battleStatesTable_[currentBattleState_]->IsFinished()) {
		// 終了処理をおこなう
		battleStatesTable_[currentBattleState_]->Finalize();
		// 状態を遷移
		if (battleStateCommonData_.requestState.has_value()) {
			// リクエストがあればその状態に遷移
			currentBattleState_ = battleStateCommonData_.requestState.value();
			battleStateCommonData_.requestState = std::nullopt;
		} else {
			// 次の行動を取得する
			currentBattleState_ = SelectWeightedBattleState();
		}
		
		Log("BossEnemy change battleState");
		// 初期化する
		battleStatesTable_[currentBattleState_]->Initialize();
	}

	// 指定した状態による更新処理
	battleStatesTable_[currentBattleState_]->Update();

	// 更新
	stateCommonData_.worldTransform->transform_ = battleStateCommonData_.transform;
}

void BossStateBattle::Exit() {
	// 実行中の行動の終了処理をおこなう
	battleStatesTable_[currentBattleState_]->Finalize();
}

BossBattleState BossStateBattle::SelectWeightedBattleState() {
	BossBattleState result = BossBattleState::kWait;

	// 全体の重みを計算する
	int32_t totalWeight = 0;
	for (const auto& item : lotteryList_) {
		totalWeight += item.weight;
	}

	if (totalWeight <= 0) { return result; }
	uint32_t randomValue = RandomGenerator::Get(int32_t(0), int32_t(totalWeight - 1));

	for (const auto& item : lotteryList_) {
		if (randomValue < item.weight) {
			result = item.behavior;
			break;
		}
		// 次の範囲へ進むために値を引く
		randomValue -= item.weight;
	}

	return result;
}
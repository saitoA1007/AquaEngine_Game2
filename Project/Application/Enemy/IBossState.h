#pragma once
#include <optional>
#include "WorldTransform.h"
#include "DebugParameter.h"
#include "BossAnimator.h"

// ボスの基本的な状態
enum class BossState {
	kIn,     // 入りの状態
	kBattle, // 戦いの状態
	kOut,    // 終わりの状態

	kMaxCount // 数
};

// 状態で共有するデータ
struct BossStateCommonData {
	// ワールド行列
	GameEngine::WorldTransform* worldTransform = nullptr;

	// 卵が壊れているか
	bool isBreakEgg = false;
	bool isDrawEgg = false;

	// hp
	int32_t hp_ = 1;

	// 状態の切り替えを管理
	std::optional<BossState> bossStateRequest = std::nullopt;

	// アニメーションの管理
	BossAnimator* animator = nullptr;

	// パラメータの保存用
	GameEngine::DebugParameter* debugParame = nullptr;
};

class IBossState {
public:
	virtual ~IBossState() = default;
	virtual void Enter() = 0;
	virtual void Update() = 0;
	virtual void Exit() = 0;
};
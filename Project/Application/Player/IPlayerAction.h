#pragma once
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Transform.h"
#include "PlayerAnimator.h"

namespace GameEngine {
	// 前方宣言
	class InputCommand;
	class DebugParameter;
}
class PlayerEffectManager;

// プレイヤーの状態
enum class PlayerState {
	kNone,        // 通常
	kJump,        // 空中
	kAttackRush,  // 突進
	kCharging,    // 突進するためのチャージ
	kAttackDown,  // 落下攻撃
	kStiffness,   // 硬直

	kMaxCount
};

// プレイヤーの共通データ
struct PlayerCommonData {
	Transform transform = { {1,1,1},{0,0,0},{0,0,0} };

	Vector3 velocity = { 0.0f,0.0f,0.0f };
	// 現在向いている方向
	Vector3 currentDir = { 0.0f,0.0f,1.0f };
	// 最終的に向く方向
	Vector3 targetDir = { 0.0f, 0.0f, 1.0f };
	// 現在の方向
	float currentYaw = 0.0f;

	// プレイヤーの状態
	PlayerState state = PlayerState::kNone;

	// プレイヤーのアニメーション管理
	PlayerAnimator* animator_ = nullptr;

	// 演出管理
	PlayerEffectManager* effectManager_ = nullptr;
};

// プレイヤーアクションの基底クラス
class IPlayerAction {
public:
	virtual ~IPlayerAction() = default;

	// 値を登録する
	virtual void RegisterParameter([[maybe_unused]] GameEngine::DebugParameter* param) {};

protected:
	// プレイヤーの共通状態
	PlayerCommonData* commonData_ = nullptr;
};
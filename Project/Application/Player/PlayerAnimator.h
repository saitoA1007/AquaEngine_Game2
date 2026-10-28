#pragma once
#include <unordered_map>
#include "Model.h"
#include "Animator.h"
#include "AnimationManager.h"

// プレイヤーのアニメーションタイプ
enum class PlayerAnimationType {
	kWalk,
	kAirMove,
	kRushAttack,
	kDownAttack,

	kMaxCount
};

class PlayerAnimator {
public:
	PlayerAnimator(GameEngine::Model* model, GameEngine::AnimationManager* animationManager);

	void Initialize();

	void Update();

public:

	// アニメーションを設定
	void StartAnimation(PlayerAnimationType type, const std::string& animeName, bool isLoop = true);
	void StartAnimation(PlayerAnimationType type, const std::string& animeName, float maxTime, bool isLoop = true);

	// 停止
	void Stop();

	// 再生
	void Start();

	// 現在のアニメーションタイプ
	PlayerAnimationType GetCurrentType() const { return currentType_; }

private:
	// アニメーション再生クラス
	GameEngine::Animator animator_;

	// アニメーションのデータ
	std::unordered_map<PlayerAnimationType, std::map<std::string, AnimationData>> animationData_;

	// 現在のタイプ
	PlayerAnimationType currentType_;

	bool isStop_ = false;
	bool isLoop_ = false;
	float maxTime_ = 1.0f;
	float timer_ = 0.0f;
};
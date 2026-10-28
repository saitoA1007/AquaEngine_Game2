#include "pch.h"
#include "GamePhaseManager.h"
#include "Application/Player/Player.h"
#include "Application/Enemy/BossEnemy.h"
#include "Application/UI/Managers/PlayUIManager.h"
#include "FPSCounter.h"
#include "EasingManager.h"
// 各フェーズ
#include "Phase/ScenePhase.h"

using namespace GameEngine;

GamePhaseManager::GamePhaseManager(GameEngine::InputCommand* inputCommand, Player* player, BossEnemy* bossEnemy,
	TitleUIManager* titleUIManager, PlayUIManager* playUIManager, GameOverUIManager* gameOverUIManager, ClearUIManager* clearUIManager,
	PauseUIManager* pauseUIManager,CameraController* cameraController, GameEngine::Dissolve* dissolve) {

	dissolve_ = dissolve;

	updateOrder_ = 0;

	commonData_.inputCommand = inputCommand;
	commonData_.timeController_ = &timeController_;

	phases_[ScenePhase::kTitle] = std::make_unique<TitlePhase>(commonData_, cameraController, titleUIManager, player);
	phases_[ScenePhase::kTutorial] = std::make_unique<TutorialPhase>(commonData_, cameraController, player, bossEnemy, playUIManager);
	phases_[ScenePhase::kPlay] = std::make_unique<PlayPhase>(commonData_, player, bossEnemy, playUIManager, cameraController);
	phases_[ScenePhase::kGameOver] = std::make_unique<GameOverPhase>(commonData_, gameOverUIManager);
	phases_[ScenePhase::kClear] = std::make_unique<ClearPhase>(commonData_, clearUIManager);
	phases_[ScenePhase::kPause] = std::make_unique<PausePhase>(commonData_, pauseUIManager);

	player_ = player;
	bossEnemy_ = bossEnemy;
	playUIManager_ = playUIManager;
}

void GamePhaseManager::Initialize() {

	commonData_.currentPhase = ScenePhase::kTitle;

	phases_[commonData_.currentPhase]->Enter();
}

void GamePhaseManager::Update() {

	// 時間の管理
	timeController_.Update();

	if (commonData_.requestPhase.has_value()) {

		// シーンの状態をリセット
		if (commonData_.resetScene) {
			isResetActive_ = true;
		} else {
			commonData_.resetScene = false;
			phases_[commonData_.currentPhase]->Exit();
			// 現在のフェーズを前のフェーズとして保存
			commonData_.SetPrePhase();
			// 現在のフェーズを変更
			commonData_.currentPhase = commonData_.requestPhase.value();
			commonData_.requestPhase = std::nullopt;
			phases_[commonData_.currentPhase]->Enter();
		}

		if (isReset_) {
			commonData_.resetScene = false;
			phases_[commonData_.currentPhase]->Exit();
			// 現在のフェーズを前のフェーズとして保存
			commonData_.SetPrePhase();
			// 現在のフェーズを変更
			commonData_.currentPhase = commonData_.requestPhase.value();
			commonData_.requestPhase = std::nullopt;
			phases_[commonData_.currentPhase]->Enter();
		}
	}

	if (isResetActive_) {
		timer_ += FpsCounter::deltaTime / maxTime_;

		float t = 0.0f;
		if (timer_ <= 0.5f) {
			float localT = timer_ / 0.5f;
			t = Lerp(0.0f, 1.0f, localT);
		} else {
			float localT = (timer_ - 0.5f) / 0.5f;
			t = Lerp(1.0f, 0.0f, localT);
		}

		dissolve_->SetThreshold(t);

		// 半分を超えたらリセット
		if (timer_ >= 0.5f) {
			if (!isReset_) {
				// リセット
				SceneReset();
				isReset_ = true;
			}
		}

		if (timer_ >= 1.0f) {
			isReset_ = false;
			isResetActive_ = false;
			timer_ = 0.0f;
			dissolve_->SetThreshold(0.0f);
		}
	}

	// 更新処理
	phases_[commonData_.currentPhase]->Update();
}

void GamePhaseManager::SceneReset() {
	// 初期化
	player_->Initialize();
	bossEnemy_->Initialize();
	playUIManager_->Initialize();
}

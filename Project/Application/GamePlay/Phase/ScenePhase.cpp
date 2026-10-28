#include "pch.h"
#include "ScenePhase.h"
#include "InputCommand.h"
#include "Application/UI/Managers/TitleUIManager.h"
#include "Application/UI/Managers/PlayUIManager.h"
#include "Application/UI/Managers/GameOverUIManager.h"
#include "Application/UI/Managers/ClearUIManager.h"
#include "Application/UI/Managers/PauseUIManager.h"
#include "Application/Player/Player.h"
#include "Application/Enemy/BossEnemy.h"
#include "Application/Camera/CameraController.h"
#include "Application/Utils/TimeController.h"

//=============================================================
// タイトル
//=============================================================

TitlePhase::TitlePhase(PhaseCommonData& commonData, CameraController* cameraController, TitleUIManager* titleUIManager, Player* player) : IScenePhase(commonData) {

	cameraController_ = cameraController;

	titleUIManager_ = titleUIManager;

	player_ = player;
}

void TitlePhase::Enter() {
	// UIを有効
	titleUIManager_->SetActive(true);
	// 初期化
	titleUIManager_->Initialize();

	// タイトルのカメラに変更
	cameraController_->SetChangeState(CameraState::kTitle);

	// プレイヤーの無効化
	player_->SetActive(false);
}

void TitlePhase::Update() {

	// 決定ボタン
	if (commonData_.inputCommand->IsCommandActive("Decision")) {
		// UIを表示させない
		titleUIManager_->SetIsDraw(false);
	}

	// タイトル文字のフェードが終わればチュートリアルシーンに移行
	if (!titleUIManager_->IsDraw() && !titleUIManager_->IsActiveFadeOut()) {
		commonData_.requestPhase = ScenePhase::kTutorial;
	}
}

void TitlePhase::Exit() {
	// UIを有効
	titleUIManager_->SetActive(false);

	// フォローカメラに変更
	cameraController_->SetChangeState(CameraState::kFollow);

	// プレイヤーを有効化
	player_->SetActive(true);
}

//=====================================================
// チュートリアル
//=====================================================

TutorialPhase::TutorialPhase(PhaseCommonData& commonData, CameraController* cameraController, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager) : IScenePhase(commonData) {

	// カメラ管理を取得
	cameraController_ = cameraController;

	// ボスを取得
	bossEnemy_ = bossEnemy;

	// プレイヤーを取得
	player_ = player;

	// UIを取得
	playUIManager_ = playUIManager;

	// UIを無効
	playUIManager_->SetActive(false);
}

void TutorialPhase::Enter() {

	// UIを有効
	playUIManager_->SetActive(true);

	// UI表示
	playUIManager_->SetIsDrawGamePlayUI(false);
	playUIManager_->SetIsDrawTutorialGuide(true);
	playUIManager_->SetIsDrawPlayGuide(true);
	playUIManager_->SetIsDrawArrowUI(true);
}

void TutorialPhase::Update() {

	if (bossEnemy_->IsBreakEgg()) {
		playUIManager_->SetIsDrawArrowUI(false);
		playUIManager_->SetIsDrawGamePlayUI(false);
		playUIManager_->SetIsDrawTutorialGuide(false);
		playUIManager_->SetIsDrawPlayGuide(false);
		// 演出カメラに変更
		cameraController_->SetChangeState(CameraState::kEnterMovie);
		// 黒帯を表示させる
		playUIManager_->SetBarActive(true);
		// プレイヤーを表示しない
		player_->SetIsDraw(false);
	} else {
		// 黒帯UIを表示
		playUIManager_->SetBarActive(cameraController_->UseLetterBoxUI());

		// ポーズ画面を開く
		if (commonData_.inputCommand->IsCommandActive("PauseAction")) {
			commonData_.requestPhase = ScenePhase::kPause;
		}
	}

	// ボスの入りのアニメーションが終わればプレイシーンに移行
	if (BossState::kBattle == bossEnemy_->GetBossState()) {
		commonData_.requestPhase = ScenePhase::kPlay;
	}
}

void TutorialPhase::Exit() {
	// 表示させる
	if (commonData_.requestPhase != ScenePhase::kPause) {
		playUIManager_->SetIsDrawGamePlayUI(true);
		playUIManager_->SetIsDrawPlayGuide(true);
		playUIManager_->SetIsDrawArrowUI(false);
		// プレイヤーを表示
		player_->SetIsDraw(true);
		// フォローカメラに変更
		cameraController_->SetChangeState(CameraState::kFollow);
	}
}

//===========================================
// プレイ
//===========================================

PlayPhase::PlayPhase(PhaseCommonData& commonData, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager, CameraController* cameraController) : IScenePhase(commonData) {
	// プレイヤー
	player_ = player;
	// ボス
	bossEnemy_ = bossEnemy;
	// プレイUIを取得
	playUIManager_ = playUIManager;
	// カメラ管理を取得
	cameraController_ = cameraController;
}

void PlayPhase::Enter() {

	if (ScenePhase::kPause != commonData_.GetPrePhase()) {
		// リセット
		playTimer_.Reset();
	}
	// 計測開始
	playTimer_.Start();

	// Hpを設定
	playUIManager_->SetCurrentBossHp(bossEnemy_->GetCurrentHp());
	playUIManager_->SetMaxBossHp(bossEnemy_->GetMaxHp());
	playUIManager_->SetCurrentPlayerHp(player_->GetCurrentHp());
	playUIManager_->SetMaxPlayerHp(player_->GetMaxHp());
}

void PlayPhase::Update() {

	// ポーズ画面を開く
	if (commonData_.inputCommand->IsCommandActive("PauseAction")) {
		commonData_.requestPhase = ScenePhase::kPause;
	}

	// 黒帯UIを表示
	playUIManager_->SetBarActive(cameraController_->UseLetterBoxUI());

	// 現在のHpを設定
	playUIManager_->SetCurrentBossHp(bossEnemy_->GetCurrentHp());
	playUIManager_->SetCurrentPlayerHp(player_->GetCurrentHp());

	// ボスが撃破されればクリアへ移行
	if (bossEnemy_->GetCurrentHp() <= 0) {
		commonData_.requestPhase = ScenePhase::kClear;
	}

	// プレイヤーが撃破されればゲームオーバーへ移行
	if (player_->GetCurrentHp() <= 0) {
		commonData_.requestPhase = ScenePhase::kGameOver;
	}

	// 計測
	playTimer_.Update();
}

void PlayPhase::Exit() {
	if (commonData_.requestPhase != ScenePhase::kPause) {
		// 計測停止
		playTimer_.Stop();
		commonData_.playTime_ = playTimer_.GetTimer();
	}
}

//=========================================================
// ポーズ
//=========================================================

PausePhase::PausePhase(PhaseCommonData& commonData, PauseUIManager* pauseUIManager) : IScenePhase(commonData) {
	pauseUIManager_ = pauseUIManager;
	pauseUIManager_->SetActive(false);
}

void PausePhase::Enter() {
	pauseUIManager_->SetActive(true);
	pauseUIManager_->Initialize();
	// 入りのアニメーション
	pauseUIManager_->Play(PauseUIManager::Phase::kIn);
}

void PausePhase::Update() {

	// 時間を停止する
	commonData_.timeController_->StartStopTime(3600.0f);

	if (!pauseUIManager_->IsAnimation()) {
		// 操作
		if (commonData_.inputCommand->IsCommandActive("SelectUp")) {

			if (selectNum_ >= 1) {
				selectNum_--;
			}
			pauseUIManager_->SetType(static_cast<PauseUIManager::SelectType>(selectNum_));
		}

		if (commonData_.inputCommand->IsCommandActive("SelectDown")) {

			if (selectNum_ <= 1) {
				selectNum_++;
			}
			pauseUIManager_->SetType(static_cast<PauseUIManager::SelectType>(selectNum_));
		}

		// 決定
		if (commonData_.inputCommand->IsCommandActive("Decision")) {

			if (PauseUIManager::SelectType::kRetry == pauseUIManager_->GetType()) {
				// やり直す
				commonData_.requestPhase = ScenePhase::kTutorial;
			} else if (PauseUIManager::SelectType::kBackTitle == pauseUIManager_->GetType()) {
				// タイトルへ戻る
				commonData_.requestPhase = ScenePhase::kTitle;
				// リセットする
				commonData_.resetScene = true;
			} else {

				// 戻る場合はアニメーション
				pauseUIManager_->Play(PauseUIManager::Phase::kOut);
			}
		}

		// 戻る
		if (commonData_.inputCommand->IsCommandActive("PauseAction")) {
			// 戻る場合はアニメーション
			pauseUIManager_->Play(PauseUIManager::Phase::kOut);
		}
	}

	// アニメーションが終了したら遷移する
	if (PauseUIManager::SelectType::kBack == pauseUIManager_->GetType()) {
		if (PauseUIManager::Phase::kOut == pauseUIManager_->GetPhase()) {
			if (!pauseUIManager_->IsAnimation()) {

				if (PauseUIManager::SelectType::kBack == pauseUIManager_->GetType()) {
					// 前のフェーズへ戻る
					commonData_.requestPhase = commonData_.GetPrePhase();
				}
			}
		}
	}
}

void PausePhase::Exit() {
	commonData_.timeController_->Reset();
	pauseUIManager_->SetActive(false);
}

//=============================================================
// ゲームオーバー
//=============================================================

GameOverPhase::GameOverPhase(PhaseCommonData& commonData, GameOverUIManager* gameOverUIManager) : IScenePhase(commonData) {

	gameOverUIManager_ = gameOverUIManager;
	gameOverUIManager_->SetActive(false);
}

void GameOverPhase::Enter() {
	// UIを有効化
	gameOverUIManager_->SetActive(true);
	// 初期化
	gameOverUIManager_->Initialize();
	gameOverUIManager_->StartEnterAnimation();
}

void GameOverPhase::Update() {

	// 操作
	if (commonData_.inputCommand->IsCommandActive("SelectUp")) {
		gameOverUIManager_->SetType(GameOverUIManager::SelectType::kRetry);
	}

	if (commonData_.inputCommand->IsCommandActive("SelectDown")) {
		gameOverUIManager_->SetType(GameOverUIManager::SelectType::kBackTitle);
	}

	// 決定
	if (commonData_.inputCommand->IsCommandActive("Decision")) {
		gameOverUIManager_->Play();

		// リセットする
		commonData_.resetScene = true;

		// 遷移させる
		if (gameOverUIManager_->GetType() == GameOverUIManager::SelectType::kRetry) {
			// やり直す
			commonData_.requestPhase = ScenePhase::kTutorial;
		} else {
			// タイトルへ戻る
			commonData_.requestPhase = ScenePhase::kTitle;
		}
	}
}

void GameOverPhase::Exit() {
	// UIを無効化
	gameOverUIManager_->SetActive(false);
}

//===========================================
// クリア
//===========================================

ClearPhase::ClearPhase(PhaseCommonData& commonData, ClearUIManager* clearUIManager) : IScenePhase(commonData) {

	clearUIManager_ = clearUIManager;
	clearUIManager_->SetActive(false);
}

void ClearPhase::Enter() {
	// UIを有効化
	clearUIManager_->SetActive(true);
	// 初期化
	clearUIManager_->Initialize();
	// アニメーション開始
	clearUIManager_->StartEnterAnimation();

	// プレイ時間を取得
	clearUIManager_->SetTime(commonData_.playTime_);
}

void ClearPhase::Update() {

	// 決定
	if (commonData_.inputCommand->IsCommandActive("Decision")) {
		// リセットする
		commonData_.resetScene = true;
		// タイトルへ戻る
		commonData_.requestPhase = ScenePhase::kTitle;
	}
}

void ClearPhase::Exit() {
	// UIを無効化
	clearUIManager_->SetActive(false);
}
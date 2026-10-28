#pragma once
#include "Application/GamePlay/IScenePhase.h"
#include "Application/Utils/Timer.h"

// 前方宣言
class Player;
class BossEnemy;
class CameraController;
class TitleUIManager;
class PlayUIManager;
class GameOverUIManager;
class ClearUIManager;
class PauseUIManager;

// タイトル
class TitlePhase : public IScenePhase {
public:
    TitlePhase(PhaseCommonData& commonData, CameraController* cameraController, TitleUIManager* titleUIManager, Player* player);
    ~TitlePhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    CameraController* cameraController_ = nullptr;

    TitleUIManager* titleUIManager_ = nullptr;

    Player* player_ = nullptr;
};

// チュートリアル
class TutorialPhase : public IScenePhase {
public:
    TutorialPhase(PhaseCommonData& commonData, CameraController* cameraController, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager);
    ~TutorialPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    // ボス
    BossEnemy* bossEnemy_ = nullptr;

    // プレイヤー
    Player* player_ = nullptr;

    // UI
    PlayUIManager* playUIManager_ = nullptr;

    // カメラ管理処理
    CameraController* cameraController_ = nullptr;

};

// プレイ
class PlayPhase : public IScenePhase {
public:
    PlayPhase(PhaseCommonData& commonData, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager, CameraController* cameraController);
    ~PlayPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    // プレイヤー
    Player* player_ = nullptr;

    // ボス
    BossEnemy* bossEnemy_ = nullptr;

    // UI
    PlayUIManager* playUIManager_ = nullptr;

    // カメラ管理処理
    CameraController* cameraController_ = nullptr;

    // プレイ時間を計測
    Timer playTimer_;
};

// ポーズシーン
class PausePhase : public IScenePhase {
public:
    PausePhase(PhaseCommonData& commonData, PauseUIManager* pauseUIManager);
    ~PausePhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    PauseUIManager* pauseUIManager_ = nullptr;

    int32_t selectNum_ = 0;
};

// ゲームオーバーシーン
class GameOverPhase : public IScenePhase {
public:
    GameOverPhase(PhaseCommonData& commonData, GameOverUIManager* gameOverUIManager);
    ~GameOverPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    GameOverUIManager* gameOverUIManager_ = nullptr;


};

// クリア
class ClearPhase : public IScenePhase {
public:
    ClearPhase(PhaseCommonData& commonData, ClearUIManager* clearUIManager);
    ~ClearPhase() = default;

    void Enter() override;

    void Update() override;

    void Exit() override;

private:
    ClearUIManager* clearUIManager_ = nullptr;


};
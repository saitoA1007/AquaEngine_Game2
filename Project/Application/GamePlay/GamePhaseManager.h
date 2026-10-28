#pragma once
#include <unordered_map>
#include <memory>
#include "IGameObject.h"
#include "IScenePhase.h"
#include "PostProcess/PostEffectData.h"
#include "Application/Utils/TimeController.h"

// 前方宣言
namespace GameEngine {
	class InputCommand;
}
class Player;
class BossEnemy;
class CameraController;
class TitleUIManager;
class PlayUIManager;
class GameOverUIManager;
class ClearUIManager;
class PauseUIManager;

class GamePhaseManager : public GameEngine::IGameObject {
public:
	GamePhaseManager(GameEngine::InputCommand* inputCommand, Player* player, BossEnemy* bossEnemy,
		TitleUIManager* titleUIManager, PlayUIManager* playUIManager, GameOverUIManager* gameOverUIManager, ClearUIManager* clearUIManager,
		PauseUIManager* pauseUIManager,CameraController* cameraController, GameEngine::Dissolve* dissolve);
	~GamePhaseManager() = default;

	void Initialize() override;

	// 更新処理
	void Update() override;

private:

	// 各フェーズ
	std::unordered_map<ScenePhase, std::unique_ptr<IScenePhase>> phases_;

	// シーン遷移に使用するポストエフェクトの変化
	GameEngine::Dissolve* dissolve_ = nullptr;
	float maxTime_ = 1.0f;
	float timer_ = 0.0f;
	bool isReset_ = false;
	bool isResetActive_ = false;

	// 共通データ
	PhaseCommonData commonData_;

	// 時間の管理
	TimeController timeController_;

	Player* player_ = nullptr;
	BossEnemy* bossEnemy_ = nullptr;
	PlayUIManager* playUIManager_ = nullptr;

private:

	/// <summary>
	/// シーンを正常な状態にリセットする
	/// </summary>
	void SceneReset();
};
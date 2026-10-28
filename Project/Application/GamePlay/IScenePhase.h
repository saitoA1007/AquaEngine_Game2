#pragma once
#include <optional>

// 前方宣言
namespace GameEngine {
    class InputCommand;
}
class TimeController;

// 状態
enum class ScenePhase {
    kTitle,    // タイトル
    kTutorial, // チュートリアル
    kPlay,     // プレイ
    kGameOver, // ゲームオーバー
    kClear,    // クリア
    kPause,    // ポーズ

    kMaxCount
};

// 各フェーズで共通データ
struct PhaseCommonData {

    // 現在のフェーズ
    ScenePhase currentPhase = ScenePhase::kTitle;

    // リクエスト
    std::optional<ScenePhase> requestPhase = std::nullopt;

    // プレイ時間
    float playTime_ = 0.0f;

    // 入力処理
    GameEngine::InputCommand* inputCommand = nullptr;

    // 時間の管理
    TimeController* timeController_ = nullptr;

    // シーンの状態をリセットする
    bool resetScene = false;

    // 前のフェーズを設定
    void SetPrePhase() {
        prePhase = currentPhase;
    }

    // 前のフェーズを取得
    ScenePhase GetPrePhase() const { return prePhase; }

private:
    // 前のフェーズ
    ScenePhase prePhase = ScenePhase::kTitle;
};

class IScenePhase {
public:
    IScenePhase(PhaseCommonData& commonData) : commonData_(commonData) {}
    virtual ~IScenePhase() = default;

    /// <summary>
    /// 入り
    /// </summary>
    virtual void Enter() = 0;

    /// <summary>
    /// 更新処理
    /// </summary>
    virtual void Update() = 0;

    /// <summary>
    /// 終わり
    /// </summary>
    virtual void Exit() = 0;

protected:
    PhaseCommonData& commonData_;
};
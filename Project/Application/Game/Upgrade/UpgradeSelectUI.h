#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include "IGameObject.h"
#include "InputCommand.h"
#include "FontManager.h"
#include "Sprite.h"
#include "Text.h"
#include "DebugParameter.h"
#include "Application/Game/Upgrade/PlayerUpgrade.h"

/// <summary>
/// 強化選択の画面。画面を暗くし、3枚のカードを横に並べる。左右でカーソルを動かし、Aで決定する。
/// 動きは止まらない時間（FpsCounter::GetDeltaTime）で進める。見た目は仮。
/// 結果はコールバックで返す。決定した時点で画面は閉じる。
/// </summary>
class UpgradeSelectUI : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 文字と板を準備する。フォントが見つからなければ例外を投げる。
    /// </summary>
    /// <param name="inputCommand">入力機能。シーンが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="fontManager">フォント取得用。コンストラクタの間だけ使い、保持しない。</param>
    /// <param name="whiteTextureHandle">板に使う白いテクスチャのハンドル</param>
    /// <param name="upgrade">名前、説明、取った回数の取得元。シーンが所有し、このオブジェクトより長く生存する。</param>
    UpgradeSelectUI(GameEngine::InputCommand* inputCommand, GameEngine::FontManager* fontManager, uint32_t whiteTextureHandle, const PlayerUpgrade* upgrade);
    ~UpgradeSelectUI() override = default;

    /// <summary>
    /// 開いている間、カーソルの移動と決定を受け付け、板と文字を更新する。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 開いている間、暗い板、カード、文字を描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 画面を開く。カーソルは左端から始める。開いてから一定時間と、Aを押していた場合は一度離すまで、決定を受け付けない。
    /// </summary>
    void Open();

    /// <summary>
    /// 画面が開いているかを返す。
    /// </summary>
    bool IsOpen() const { return isOpen_; }

    /// <summary>
    /// 項目が決定されたときに呼ぶ関数を設定する。呼ぶ時点で画面は閉じており、この中でOpenを呼んでよい。
    /// </summary>
    /// <param name="callback">選ばれた項目を受け取る関数。呼び出し先がこのオブジェクトより長く生存する必要がある。</param>
    void SetSelectedCallback(const std::function<void(UpgradeType)>& callback) { selectedCallback_ = callback; }

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 板と文字を作る。
    /// </summary>
    void CreateViews(const GameEngine::Font* font, uint32_t whiteTextureHandle);

    /// <summary>
    /// 左右の入力でカーソルを動かす。一度中立に戻すまで、次の移動をしない。
    /// </summary>
    void UpdateCursor();

    /// <summary>
    /// 決定の入力を受け付けられる状態かを更新し、受け付けられるなら決定を処理する。
    /// </summary>
    /// <param name="deltaTime">止まらない時間の経過（秒）</param>
    void UpdateConfirm(float deltaTime);

    /// <summary>
    /// 取った回数などの文字を、現在の値へ更新する。
    /// </summary>
    void RefreshTexts();

    /// <summary>
    /// カーソルの位置に合わせて、カードの色と大きさを更新する。
    /// </summary>
    void UpdateCardViews();

private:
    // 調整値
    float confirmLockTime_ = 0.4f; // 開いてから決定を受け付けない時間（実時間の秒）

    // 定数
    static constexpr int32_t kCardCount = PlayerUpgrade::kTypeCount;
    static constexpr float kScreenWidth = 1280.0f;
    static constexpr float kScreenHeight = 720.0f;
    static constexpr float kCardWidth = 300.0f;
    static constexpr float kCardHeight = 380.0f;
    static constexpr float kCardGap = 40.0f;
    static constexpr float kCardCenterY = 380.0f;
    inline static const char* kFontName = "源直ゴシック EMG 2 - Medium"; // フォントの登録名
    inline static const char* kConfirmCommand = "UpgradeConfirm";        // 決定のコマンド（GameSceneが登録する）
    inline static const char* kHoldCommand = "IceMistCharge";            // Aを押している間のコマンド（開いた時点で押していたかの判定に使う）

    // 状態
    bool isOpen_ = false;          // 画面が開いているか
    int32_t cursor_ = 0;           // カーソルの位置（カードの番号）
    float openTimer_ = 0.0f;       // 開いてからの実時間（秒）
    bool needReleaseA_ = false;    // Aを一度離すまで決定を受け付けないか
    bool needNeutral_ = false;     // 左右を一度中立に戻すまで移動しないか

    // 参照（所有しない）
    GameEngine::InputCommand* inputCommand_ = nullptr;
    const PlayerUpgrade* upgrade_ = nullptr;
    std::function<void(UpgradeType)> selectedCallback_;

    // 描画
    std::unique_ptr<GameEngine::Sprite> dimSprite_;                               // 画面を暗くする板
    std::array<std::unique_ptr<GameEngine::Sprite>, kCardCount> cardSprites_;     // カードの板
    std::unique_ptr<GameEngine::Text> titleText_;                                 // 見出し
    std::unique_ptr<GameEngine::Text> hintText_;                                  // 操作の案内
    std::array<std::unique_ptr<GameEngine::Text>, kCardCount> nameTexts_;         // カードの名前
    std::array<std::unique_ptr<GameEngine::Text>, kCardCount> descriptionTexts_;  // カードの説明
    std::array<std::unique_ptr<GameEngine::Text>, kCardCount> countTexts_;        // 取った回数

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

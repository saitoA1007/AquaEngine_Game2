#pragma once
#include <memory>
#include "IGameObject.h"
#include "InputCommand.h"
#include "FontManager.h"
#include "Sprite.h"
#include "Text.h"
#include "DebugParameter.h"
#include "Vector2.h"
#include "Vector4.h"

/// <summary>
/// ゲームの終わり方。見出しの文字と色を切り替える。
/// </summary>
enum class GameEndType {
    kGameOver,  // HPが0になった
    kClear,     // クリアした
};

/// <summary>
/// 終わったあとに選ばれた行き先。
/// </summary>
enum class GameEndChoice {
    kNone,   // まだ選ばれていない
    kRetry,  // やり直す（Gameシーンを最初から）
    kTitle,  // タイトルへ
};

/// <summary>
/// ゲームの終わりの表示。画面を暗くして見出し（GAME OVER / CLEAR）を出し、待ち時間のあとに
/// 「やり直す」「タイトルへ」を縦に並べて、上下で選び、決定で行き先を確定する。
/// 時間は止まらない時間（FpsCounter::GetDeltaTime）で数える。閉じているあいだは何もしない。
/// 選ばれた行き先はGetChoiceで返し、シーンの終了と遷移先はGameSceneが決める。見た目は仮。
/// </summary>
class GameOverView : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 文字と板を準備する。フォントが見つからなければ例外を投げる。
    /// </summary>
    /// <param name="inputCommand">入力機能。シーンが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="fontManager">フォント取得用。コンストラクタの間だけ使い、保持しない。</param>
    /// <param name="whiteTextureHandle">暗くする板の画像のハンドル</param>
    GameOverView(GameEngine::InputCommand* inputCommand, GameEngine::FontManager* fontManager, uint32_t whiteTextureHandle);
    ~GameOverView() override = default;

    /// <summary>
    /// 開いている間、待ち時間を数え、選択と決定の入力を受け付け、表示を更新する。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 開いている間、暗い板と文字を描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 表示を始める。待ち時間を0から数え、「やり直す」を選んだ状態にする。
    /// </summary>
    /// <param name="endType">ゲームの終わり方。見出しが決まる。</param>
    void Open(GameEndType endType);

    /// <summary>
    /// 表示中かを返す。
    /// </summary>
    bool IsOpen() const { return isOpen_; }

    /// <summary>
    /// 選ばれた行き先を返す。待ち時間が過ぎて選択肢が出てからの決定だけが対象。決まるまではkNone。
    /// </summary>
    GameEndChoice GetChoice() const { return choice_; }

private:
    // 選択肢の並び（上から）
    enum class Option {
        kRetry,
        kTitle,
    };

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 文字と板を作る。
    /// </summary>
    void CreateViews(const GameEngine::Font* font, uint32_t whiteTextureHandle);

    /// <summary>
    /// 上下の入力（キー・十字キーは押した瞬間、スティックは倒した瞬間）で選択を1段動かし、
    /// 決定の入力があれば行き先を確定する。選択肢が出ていないあいだと、確定後は何もしない。
    /// </summary>
    void UpdateSelection();

    /// <summary>
    /// スティックの上下を、前のフレームと比べて倒した瞬間だけ読む。毎フレーム呼んで前の状態を更新する。
    /// </summary>
    /// <param name="isUp">倒した瞬間が上向きならtrue</param>
    /// <param name="isDown">倒した瞬間が下向きならtrue</param>
    void ReadStickEdge(bool& isUp, bool& isDown);

    /// <summary>
    /// 経過時間に合わせて、板と文字の透明度と、選択肢の色を決めて更新する。
    /// </summary>
    void UpdateViews();

private:
    // 調整値
    float retryWaitTime_ = 1.0f;                          // 選択肢を出して決定を受け付けるまでの待ち時間（秒）
    float fadeInTime_ = 0.4f;                             // 画面と文字が現れるまでの時間（秒）
    float dimAlpha_ = 0.6f;                               // 画面を暗くする板の濃さ
    Vector2 titlePosition_ = { 640.0f, 260.0f };          // 見出しの中心（画面座標）
    float titleSize_ = 96.0f;                             // 見出しの文字の大きさ（px）
    Vector4 gameOverColor_ = { 1.0f, 0.45f, 0.45f, 1.0f };  // 「GAME OVER」の色
    Vector4 clearColor_ = { 0.55f, 0.9f, 1.0f, 1.0f };      // 「CLEAR」の色（水色）
    Vector2 optionPosition_ = { 640.0f, 420.0f };         // 一番上の選択肢の中心（画面座標）
    float optionSpacing_ = 70.0f;                         // 選択肢の縦の間隔（px）
    float optionSize_ = 44.0f;                            // 選択肢の文字の大きさ（px）
    Vector4 selectedColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };       // 選んでいる項目の色
    Vector4 unselectedColor_ = { 0.55f, 0.58f, 0.65f, 1.0f };  // 選んでいない項目の色
    float stickThreshold_ = 0.5f;                         // スティックを倒したと見なすしきい値（0〜1）

    // 定数
    static constexpr float kScreenWidth = 1280.0f;
    static constexpr float kScreenHeight = 720.0f;
    inline static const char* kFontName = "源直ゴシック EMG 2 - Medium"; // フォントの登録名
    inline static const char* kMenuUpCommand = "MenuUp";                 // 上へ動かすコマンド（GameSceneが登録する）
    inline static const char* kMenuDownCommand = "MenuDown";             // 下へ動かすコマンド（GameSceneが登録する）
    inline static const char* kDecideCommand = "Decide";                 // 決定のコマンド（GameSceneが登録する）

    // 状態
    bool isOpen_ = false;                       // 表示中か
    GameEndChoice choice_ = GameEndChoice::kNone;  // 選ばれた行き先
    Option selectedOption_ = Option::kRetry;    // 今選んでいる項目
    GameEndType endType_ = GameEndType::kGameOver; // 終わり方
    float timer_ = 0.0f;                        // 開いてからの経過時間（秒）
    float previousStickY_ = 0.0f;               // 前のフレームの左スティックの上下（倒した瞬間の判定に使う）

    // 参照（所有しない）
    GameEngine::InputCommand* inputCommand_ = nullptr;

    // 描画
    std::unique_ptr<GameEngine::Sprite> dimSprite_;     // 画面を暗くする板
    std::unique_ptr<GameEngine::Text> titleText_;       // 見出し
    std::unique_ptr<GameEngine::Text> retryText_;       // 「やり直す」
    std::unique_ptr<GameEngine::Text> backToTitleText_; // 「タイトルへ」

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

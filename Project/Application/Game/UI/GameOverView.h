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
/// ゲームオーバーの表示。画面を暗くして「GAME OVER」を出し、待ち時間のあとに「Aで再開」を出して、再開の入力を受け付ける。
/// 時間は止まらない時間（FpsCounter::GetDeltaTime）で数える。閉じているあいだは何もしない。
/// 再開するかどうかはIsRetryRequestedで返し、シーンの終了はGameSceneが決める。見た目は仮。
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
    /// 開いている間、待ち時間を数え、再開の入力を受け付け、表示を更新する。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 開いている間、暗い板と文字を描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 表示を始める。待ち時間を0から数える。
    /// </summary>
    void Open();

    /// <summary>
    /// 表示中かを返す。
    /// </summary>
    bool IsOpen() const { return isOpen_; }

    /// <summary>
    /// 再開の入力があったかを返す。待ち時間が過ぎて「Aで再開」が出てからの入力だけが対象。
    /// </summary>
    bool IsRetryRequested() const { return isRetryRequested_; }

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
    /// 経過時間に合わせて、板と文字の透明度を決めて更新する。
    /// </summary>
    void UpdateViews();

private:
    // 調整値
    float retryWaitTime_ = 1.0f;                          // 「Aで再開」を出して再開を受け付けるまでの待ち時間（秒）
    float fadeInTime_ = 0.4f;                             // 画面と文字が現れるまでの時間（秒）
    float dimAlpha_ = 0.6f;                               // 画面を暗くする板の濃さ
    Vector2 titlePosition_ = { 640.0f, 300.0f };          // 「GAME OVER」の中心（画面座標）
    float titleSize_ = 96.0f;                             // 「GAME OVER」の文字の大きさ（px）
    Vector4 titleColor_ = { 1.0f, 0.45f, 0.45f, 1.0f };   // 「GAME OVER」の色
    Vector2 promptPosition_ = { 640.0f, 440.0f };         // 「Aで再開」の中心（画面座標）
    float promptSize_ = 44.0f;                            // 「Aで再開」の文字の大きさ（px）
    Vector4 promptColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };    // 「Aで再開」の色

    // 定数
    static constexpr float kScreenWidth = 1280.0f;
    static constexpr float kScreenHeight = 720.0f;
    inline static const char* kFontName = "源直ゴシック EMG 2 - Medium"; // フォントの登録名
    inline static const char* kRetryCommand = "Retry";                   // 再開のコマンド（GameSceneが登録する）

    // 状態
    bool isOpen_ = false;             // 表示中か
    bool isRetryRequested_ = false;   // 再開の入力があったか
    float timer_ = 0.0f;              // 開いてからの経過時間（秒）

    // 参照（所有しない）
    GameEngine::InputCommand* inputCommand_ = nullptr;

    // 描画
    std::unique_ptr<GameEngine::Sprite> dimSprite_;     // 画面を暗くする板
    std::unique_ptr<GameEngine::Text> titleText_;       // 「GAME OVER」
    std::unique_ptr<GameEngine::Text> promptText_;      // 「Aで再開」

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

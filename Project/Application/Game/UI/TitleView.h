#pragma once
#include <memory>
#include <string>
#include "IGameObject.h"
#include "FontManager.h"
#include "Sprite.h"
#include "DebugParameter.h"
#include "Vector2.h"
#include "Vector4.h"
#include "OutlinedText.h"

/// <summary>
/// タイトル画面の表示。暗い青の背景の板、作品名（仮）、「Aでスタート」を出す。
/// 入力は見ず、シーンの切り替えはTitleSceneが決める。点滅などの演出は持たない。見た目は仮。
/// </summary>
class TitleView : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 文字と板を準備する。フォントが見つからなければ例外を投げる。
    /// </summary>
    /// <param name="fontManager">フォント取得用。コンストラクタの間だけ使い、保持しない。</param>
    /// <param name="whiteTextureHandle">背景の板の画像のハンドル</param>
    TitleView(GameEngine::FontManager* fontManager, uint32_t whiteTextureHandle);
    ~TitleView() override = default;

    /// <summary>
    /// 調整値を板と文字へ反映する。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 背景の板と文字を描画キューへ登録する。
    /// </summary>
    void Draw() override;

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 板と文字を作る。
    /// </summary>
    void CreateViews(const GameEngine::Font* font, uint32_t whiteTextureHandle);

private:
    // 調整値
    Vector4 backgroundColor_ = { 0.03f, 0.08f, 0.2f, 1.0f };   // 背景の板の色（暗い青）
    Vector2 titlePosition_ = { 640.0f, 270.0f };                // 作品名の中心（画面座標）
    float titleSize_ = 96.0f;                                   // 作品名の文字の大きさ（px）
    Vector4 titleColor_ = { 0.75f, 0.93f, 1.0f, 1.0f };         // 作品名の色
    Vector2 promptPosition_ = { 640.0f, 500.0f };               // 「Aでスタート」の中心（画面座標）
    float promptSize_ = 44.0f;                                  // 「Aでスタート」の文字の大きさ（px）
    Vector4 promptColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };          // 「Aでスタート」の色

    // 定数
    static constexpr float kScreenWidth = 1280.0f;
    static constexpr float kScreenHeight = 720.0f;
    static constexpr uint32_t kTitleMaxLength = 16;   // 作品名の最大文字数
    static constexpr uint32_t kPromptMaxLength = 12;  // 「Aでスタート」の最大文字数
    inline static const char* kFontName = "源直ゴシック EMG 2 - Medium";  // フォントの登録名
    inline static const char* kProvisionalTitle = "氷のシューティング";   // 仮の作品名（作品名は未定）
    inline static const char* kPromptText = "Aでスタート";

    // 描画
    std::unique_ptr<GameEngine::Sprite> backgroundSprite_;  // 全画面の背景の板
    std::unique_ptr<OutlinedText> titleText_;               // 作品名
    std::unique_ptr<OutlinedText> promptText_;              // 「Aでスタート」

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

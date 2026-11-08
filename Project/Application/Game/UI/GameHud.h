#pragma once
#include <memory>
#include "IGameObject.h"
#include "FontManager.h"
#include "Text.h"
#include "DebugParameter.h"
#include "Vector2.h"
#include "Vector4.h"
#include "Application/Game/UI/HudInput.h"
#include "Application/Game/UI/HpIconView.h"

/// <summary>
/// 常時表示のUI。左上にHPアイコン、右上にフェーズの文字を出す。
/// ゲームのクラスを知らず、GameSceneから受け取ったHudInputだけで表示する。
/// 演出は止まらない時間で進める。見た目は仮（丸の画像を塗る）。
/// </summary>
class GameHud : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 文字を準備する。フォントが見つからなければ例外を投げる。
    /// </summary>
    /// <param name="fontManager">フォント取得用。コンストラクタの間だけ使い、保持しない。</param>
    /// <param name="circleTextureHandle">HPアイコンの画像のハンドル（仮素材）</param>
    GameHud(GameEngine::FontManager* fontManager, uint32_t circleTextureHandle);
    ~GameHud() override = default;

    /// <summary>
    /// 調整値を反映し、HPアイコンの演出を進める。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 最新のHudInputで位置・大きさ・文字を決めて、描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 表示する値を設定する。毎フレーム、GameSceneの更新の最後に呼ぶ。
    /// </summary>
    /// <param name="input">表示する値</param>
    void SetInput(const HudInput& input);

    /// <summary>
    /// 被弾の演出を始める（HPアイコンが弾ける）。
    /// </summary>
    void PlayDamage() { hpIconView_->PlayDamage(); }

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 文字を作る。
    /// </summary>
    void CreateViews(const GameEngine::Font* font);

    /// <summary>
    /// フェーズの文字を、値が変わったときだけ書き換える。
    /// </summary>
    void RefreshTexts();

    /// <summary>
    /// 調整値に合わせて、文字の位置・大きさ・色を更新する。
    /// </summary>
    void UpdateViews();

private:
    // 調整値
    Vector2 phaseTextPosition_ = { 1240.0f, 40.0f };      // 「PHASE」の文字の右上（画面座標）
    float phaseTextSize_ = 36.0f;                         // 「PHASE」の文字の大きさ（px）
    Vector4 textColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };      // 文字の色（フェーズはボーナスでもボスでも同じ）

    // 定数
    inline static const char* kFontName = "源直ゴシック EMG 2 - Medium"; // フォントの登録名

    // 状態
    HudInput input_;                  // 最新の表示する値
    int32_t shownPhase_ = -1;         // 文字へ書いたフェーズ（-1は未設定）

    // 描画
    std::unique_ptr<HpIconView> hpIconView_;                  // HPアイコン
    std::unique_ptr<GameEngine::Text> phaseText_;             // 「PHASE 4」

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

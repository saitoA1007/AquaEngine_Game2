#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include "Font.h"
#include "RenderQueue.h"
#include "Text.h"
#include "Vector2.h"
#include "Vector4.h"

/// <summary>
/// 縁取りと影のついた文字。同じ文字を縁の色で8方向にずらして重ね、右下にずらした影を1つ付ける。
/// 縁の太さと影のずれは、文字の大きさに対する割合で決める。描く順は、影、縁、本体。
/// </summary>
class OutlinedText {
public:
    // 見た目の設定
    struct Style {
        Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };           // 文字の色
        Vector4 outlineColor = { 0.05f, 0.08f, 0.2f, 1.0f };  // 縁の色
        Vector4 shadowColor = { 0.0f, 0.0f, 0.0f, 0.5f };     // 影の色
        float outlineRatio = 0.06f;                           // 縁の太さ（文字の大きさに対する割合）
        Vector2 shadowRatio = { 0.1f, 0.1f };                 // 影のずれ（文字の大きさに対する割合）
    };

public:
    /// <summary>
    /// 本体・縁・影の文字を作る。
    /// </summary>
    /// <param name="font">使用するフォント。呼び出し元が所有する。</param>
    /// <param name="text">表示する文字列(UTF-8)</param>
    /// <param name="maxLength">表示できる最大文字数</param>
    OutlinedText(const GameEngine::Font* font, const std::string& text, uint32_t maxLength);
    ~OutlinedText() = default;

    /// <summary>
    /// 表示する文字列を設定する。
    /// </summary>
    void SetText(const std::string& text);

    /// <summary>
    /// 文字の大きさ（px）を設定する。GetSizeはこの大きさでの文字全体の大きさを返す。
    /// </summary>
    void SetFontSize(float fontSize);

    /// <summary>
    /// 縁と影を含めない、文字全体の大きさ（px）。SetFontSizeの後で正しい値になる。
    /// </summary>
    Vector2 GetSize() const { return body_->GetSize(); }

    /// <summary>
    /// 設定した文字の大きさ（px）。
    /// </summary>
    float GetFontSize() const { return fontSize_; }

    /// <summary>
    /// 基準点（0〜1。0.5で中心、(0,1)で左下）を設定する。
    /// </summary>
    void SetAnchor(const Vector2& anchor);

    /// <summary>
    /// 位置・大きさ・色を本体、縁、影へ反映する。
    /// </summary>
    /// <param name="position">基準点の画面座標</param>
    /// <param name="style">色・縁の太さ・影のずれ</param>
    /// <param name="alpha">全体の透明度の倍率（0〜1）</param>
    void Update(const Vector2& position, const Style& style, float alpha);

    /// <summary>
    /// 影、縁、本体の順に描画キューへ登録する。
    /// </summary>
    /// <param name="renderQueue">登録先。呼び出し元が所有する。</param>
    void Submit(GameEngine::RenderQueue* renderQueue);

private:
    // 縁をずらす方向の数
    static constexpr int32_t kOutlineCount = 8;

    // 文字の大きさ
    float fontSize_ = 0.0f;

    // 文字（影、縁、本体）
    std::unique_ptr<GameEngine::Text> shadow_;
    std::array<std::unique_ptr<GameEngine::Text>, kOutlineCount> outlines_;
    std::unique_ptr<GameEngine::Text> body_;
};

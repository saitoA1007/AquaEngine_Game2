#include "pch.h"
#include "TitleView.h"
#include <stdexcept>
using namespace GameEngine;

TitleView::TitleView(FontManager* fontManager, uint32_t whiteTextureHandle) {
    // フォントが見つからなければ例外にする
    const Font* font = fontManager->GetFont(kFontName);
    if (font == nullptr) {
        throw std::runtime_error(std::string("font not found: ") + kFontName);
    }

    RegisterDebugParameters();
    CreateViews(font, whiteTextureHandle);
}

void TitleView::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("TitleView");
    debugParam_->Register("backgroundColor", backgroundColor_, 0);
    debugParam_->Register("titlePosition", titlePosition_, 1);
    debugParam_->Register("titleSize", titleSize_, 2);
    debugParam_->Register("titleColor", titleColor_, 3);
    debugParam_->Register("promptPosition", promptPosition_, 4);
    debugParam_->Register("promptSize", promptSize_, 5);
    debugParam_->Register("promptColor", promptColor_, 6);
    debugParam_->Apply();
}

void TitleView::CreateViews(const Font* font, uint32_t whiteTextureHandle) {
    // 画面全体を覆う背景の板（白い画像に色を掛ける）
    backgroundSprite_ = std::make_unique<Sprite>(Vector2{ 0.0f, 0.0f }, Vector2{ kScreenWidth, kScreenHeight }, Vector2{ 0.0f, 0.0f }, backgroundColor_);
    backgroundSprite_->textureHandle_ = whiteTextureHandle;

    titleText_ = std::make_unique<OutlinedText>(font, kProvisionalTitle, kTitleMaxLength);
    titleText_->SetAnchor({ 0.5f, 0.5f });
    promptText_ = std::make_unique<OutlinedText>(font, kPromptText, kPromptMaxLength);
    promptText_->SetAnchor({ 0.5f, 0.5f });
}

void TitleView::Update() {
    debugParam_->ApplyIfDirty();

    backgroundSprite_->color_ = backgroundColor_;
    backgroundSprite_->Update();

    // 色だけ調整値で決め、縁と影は既定のまま使う
    OutlinedText::Style titleStyle;
    titleStyle.color = titleColor_;
    titleText_->SetFontSize(titleSize_);
    titleText_->Update(titlePosition_, titleStyle, 1.0f);

    OutlinedText::Style promptStyle;
    promptStyle.color = promptColor_;
    promptText_->SetFontSize(promptSize_);
    promptText_->Update(promptPosition_, promptStyle, 1.0f);
}

void TitleView::Draw() {
    renderQueue_->SubmitSprite(backgroundSprite_.get());
    titleText_->Submit(renderQueue_);
    promptText_->Submit(renderQueue_);
}

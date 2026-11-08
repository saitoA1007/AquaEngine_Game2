#include "pch.h"
#include "GameOverView.h"
#include "FPSCounter.h"
#include <algorithm>
#include <stdexcept>
#include <string>
using namespace GameEngine;

GameOverView::GameOverView(InputCommand* inputCommand, FontManager* fontManager, uint32_t whiteTextureHandle)
    : inputCommand_(inputCommand) {
    // フォントが見つからなければ例外にする
    const Font* font = fontManager->GetFont(kFontName);
    if (font == nullptr) {
        throw std::runtime_error(std::string("font not found: ") + kFontName);
    }

    RegisterDebugParameters();
    CreateViews(font, whiteTextureHandle);
}

void GameOverView::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("GameOverView");
    debugParam_->Register("retryWaitTime", retryWaitTime_, 0);
    debugParam_->Register("fadeInTime", fadeInTime_, 1);
    debugParam_->Register("dimAlpha", dimAlpha_, 2);
    debugParam_->Register("titlePosition", titlePosition_, 3);
    debugParam_->Register("titleSize", titleSize_, 4);
    debugParam_->Register("titleColor", titleColor_, 5);
    debugParam_->Register("promptPosition", promptPosition_, 6);
    debugParam_->Register("promptSize", promptSize_, 7);
    debugParam_->Register("promptColor", promptColor_, 8);
    debugParam_->Apply();
}

void GameOverView::CreateViews(const Font* font, uint32_t whiteTextureHandle) {
    // 画面全体を覆う暗い板
    dimSprite_ = std::make_unique<Sprite>(Vector2{ 0.0f, 0.0f }, Vector2{ kScreenWidth, kScreenHeight }, Vector2{ 0.0f, 0.0f }, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f });
    dimSprite_->textureHandle_ = whiteTextureHandle;

    titleText_ = std::make_unique<Text>(font, "GAME OVER", 12);
    titleText_->anchorPoint_ = { 0.5f, 0.5f };
    promptText_ = std::make_unique<Text>(font, "Aで再開", 8);
    promptText_->anchorPoint_ = { 0.5f, 0.5f };
}

void GameOverView::Open() {
    isOpen_ = true;
    isRetryRequested_ = false;
    timer_ = 0.0f;
}

void GameOverView::Update() {
    debugParam_->ApplyIfDirty();
    if (!isOpen_) {
        return;
    }

    timer_ += FpsCounter::GetDeltaTime();

    // 「Aで再開」が出てから、再開の入力を受け付ける（押しっぱなしで即座に再開しないため）
    if (timer_ >= retryWaitTime_ && inputCommand_->IsCommandActive(kRetryCommand)) {
        isRetryRequested_ = true;
    }

    UpdateViews();
}

void GameOverView::UpdateViews() {
    // 板と「GAME OVER」は開いてから現れ、「Aで再開」は待ち時間のあとに現れる
    float fade = (std::min)(timer_ / (std::max)(fadeInTime_, 0.01f), 1.0f);
    float promptFade = (std::clamp)((timer_ - retryWaitTime_) / (std::max)(fadeInTime_, 0.01f), 0.0f, 1.0f);

    dimSprite_->color_ = { 0.0f, 0.0f, 0.0f, dimAlpha_ * fade };
    dimSprite_->Update();

    titleText_->position_ = titlePosition_;
    titleText_->scale_ = { titleSize_, titleSize_ };
    titleText_->color_ = { titleColor_.x, titleColor_.y, titleColor_.z, titleColor_.w * fade };
    titleText_->Update();

    promptText_->position_ = promptPosition_;
    promptText_->scale_ = { promptSize_, promptSize_ };
    promptText_->color_ = { promptColor_.x, promptColor_.y, promptColor_.z, promptColor_.w * promptFade };
    promptText_->Update();
}

void GameOverView::Draw() {
    if (!isOpen_) {
        return;
    }

    renderQueue_->SubmitSprite(dimSprite_.get());
    renderQueue_->SubmitText(titleText_.get());
    if (timer_ >= retryWaitTime_) {
        renderQueue_->SubmitText(promptText_.get());
    }
}

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
    debugParam_->Register("gameOverColor", gameOverColor_, 5);
    debugParam_->Register("clearColor", clearColor_, 6);
    debugParam_->Register("optionPosition", optionPosition_, 7);
    debugParam_->Register("optionSpacing", optionSpacing_, 8);
    debugParam_->Register("optionSize", optionSize_, 9);
    debugParam_->Register("selectedColor", selectedColor_, 10);
    debugParam_->Register("unselectedColor", unselectedColor_, 11);
    debugParam_->Register("stickThreshold", stickThreshold_, 12);
    debugParam_->Apply();
}

void GameOverView::CreateViews(const Font* font, uint32_t whiteTextureHandle) {
    // 画面全体を覆う暗い板
    dimSprite_ = std::make_unique<Sprite>(Vector2{ 0.0f, 0.0f }, Vector2{ kScreenWidth, kScreenHeight }, Vector2{ 0.0f, 0.0f }, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f });
    dimSprite_->textureHandle_ = whiteTextureHandle;

    // 見出しの文字はOpenで切り替える。最大文字数は「GAME OVER」に合わせる
    titleText_ = std::make_unique<Text>(font, "GAME OVER", 12);
    titleText_->anchorPoint_ = { 0.5f, 0.5f };
    retryText_ = std::make_unique<Text>(font, "やり直す", 8);
    retryText_->anchorPoint_ = { 0.5f, 0.5f };
    backToTitleText_ = std::make_unique<Text>(font, "タイトルへ", 8);
    backToTitleText_->anchorPoint_ = { 0.5f, 0.5f };
}

void GameOverView::Open(GameEndType endType) {
    isOpen_ = true;
    choice_ = GameEndChoice::kNone;
    selectedOption_ = Option::kRetry;
    endType_ = endType;
    timer_ = 0.0f;

    // 開いた時点のスティックの状態を覚える。倒したまま開いても、すぐには動かさない
    previousStickY_ = inputCommand_->GetLeftStick().y;

    titleText_->SetText(endType == GameEndType::kClear ? "CLEAR" : "GAME OVER");
}

void GameOverView::Update() {
    debugParam_->ApplyIfDirty();
    if (!isOpen_) {
        return;
    }

    timer_ += FpsCounter::GetDeltaTime();

    UpdateSelection();
    UpdateViews();
}

void GameOverView::UpdateSelection() {
    // スティックの前の状態は、選択肢が出る前も毎フレーム更新する
    bool isStickUp = false;
    bool isStickDown = false;
    ReadStickEdge(isStickUp, isStickDown);

    // 選択肢が出てから、確定するまでの間だけ入力を受け付ける（押しっぱなしで即座に決まらないため）
    if (timer_ < retryWaitTime_ || choice_ != GameEndChoice::kNone) {
        return;
    }

    bool isUp = isStickUp || inputCommand_->IsCommandActive(kMenuUpCommand);
    bool isDown = isStickDown || inputCommand_->IsCommandActive(kMenuDownCommand);

    // 端で止める。1回の入力で1段だけ動かす
    if (isUp) {
        selectedOption_ = Option::kRetry;
    } else if (isDown) {
        selectedOption_ = Option::kTitle;
    }

    // 決定で、選んでいる項目を行き先として確定する
    if (inputCommand_->IsCommandActive(kDecideCommand)) {
        choice_ = (selectedOption_ == Option::kRetry) ? GameEndChoice::kRetry : GameEndChoice::kTitle;
    }
}

void GameOverView::ReadStickEdge(bool& isUp, bool& isDown) {
    float stickY = inputCommand_->GetLeftStick().y;

    // しきい値を越えた瞬間だけ（前のフレームは越えていなかった）を倒した瞬間とする
    isUp = stickY > stickThreshold_ && previousStickY_ <= stickThreshold_;
    isDown = stickY < -stickThreshold_ && previousStickY_ >= -stickThreshold_;

    previousStickY_ = stickY;
}

void GameOverView::UpdateViews() {
    // 板と見出しは開いてから現れ、選択肢は待ち時間のあとに現れる
    float fade = (std::min)(timer_ / (std::max)(fadeInTime_, 0.01f), 1.0f);
    float optionFade = (std::clamp)((timer_ - retryWaitTime_) / (std::max)(fadeInTime_, 0.01f), 0.0f, 1.0f);

    dimSprite_->color_ = { 0.0f, 0.0f, 0.0f, dimAlpha_ * fade };
    dimSprite_->Update();

    const Vector4& titleColor = (endType_ == GameEndType::kClear) ? clearColor_ : gameOverColor_;
    titleText_->position_ = titlePosition_;
    titleText_->scale_ = { titleSize_, titleSize_ };
    titleText_->color_ = { titleColor.x, titleColor.y, titleColor.z, titleColor.w * fade };
    titleText_->Update();

    // 選んでいる項目と選んでいない項目は色で区別する
    const Vector4& retryColor = (selectedOption_ == Option::kRetry) ? selectedColor_ : unselectedColor_;
    const Vector4& backToTitleColor = (selectedOption_ == Option::kTitle) ? selectedColor_ : unselectedColor_;

    retryText_->position_ = optionPosition_;
    retryText_->scale_ = { optionSize_, optionSize_ };
    retryText_->color_ = { retryColor.x, retryColor.y, retryColor.z, retryColor.w * optionFade };
    retryText_->Update();

    backToTitleText_->position_ = { optionPosition_.x, optionPosition_.y + optionSpacing_ };
    backToTitleText_->scale_ = { optionSize_, optionSize_ };
    backToTitleText_->color_ = { backToTitleColor.x, backToTitleColor.y, backToTitleColor.z, backToTitleColor.w * optionFade };
    backToTitleText_->Update();
}

void GameOverView::Draw() {
    if (!isOpen_) {
        return;
    }

    renderQueue_->SubmitSprite(dimSprite_.get());
    renderQueue_->SubmitText(titleText_.get());
    if (timer_ >= retryWaitTime_) {
        renderQueue_->SubmitText(retryText_.get());
        renderQueue_->SubmitText(backToTitleText_.get());
    }
}

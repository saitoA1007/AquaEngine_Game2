#include "pch.h"
#include "UpgradeSelectUI.h"
#include "FPSCounter.h"
#include <algorithm>
#include <stdexcept>
#include <string>
using namespace GameEngine;

UpgradeSelectUI::UpgradeSelectUI(InputCommand* inputCommand, FontManager* fontManager, uint32_t whiteTextureHandle, const PlayerUpgrade* upgrade)
    : inputCommand_(inputCommand), upgrade_(upgrade) {

    // フォントが見つからなければ例外にする
    const Font* font = fontManager->GetFont(kFontName);
    if (font == nullptr) {
        throw std::runtime_error(std::string("font not found: ") + kFontName);
    }

    RegisterDebugParameters();
    CreateViews(font, whiteTextureHandle);
}

void UpgradeSelectUI::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("UpgradeSelectUI");
    debugParam_->Register("confirmLockTime", confirmLockTime_, 0);
    debugParam_->Apply();
}

void UpgradeSelectUI::CreateViews(const Font* font, uint32_t whiteTextureHandle) {
    // 画面全体を覆う暗い板
    dimSprite_ = std::make_unique<Sprite>(Vector2{ 0.0f, 0.0f }, Vector2{ kScreenWidth, kScreenHeight }, Vector2{ 0.0f, 0.0f }, Vector4{ 0.0f, 0.0f, 0.0f, 0.65f });
    dimSprite_->textureHandle_ = whiteTextureHandle;

    // 見出しと操作の案内
    titleText_ = std::make_unique<Text>(font, "強化を選んでください", 16);
    titleText_->position_ = { kScreenWidth * 0.5f, 90.0f };
    titleText_->scale_ = { 44.0f, 44.0f };
    titleText_->anchorPoint_ = { 0.5f, 0.5f };

    hintText_ = std::make_unique<Text>(font, "左右で選ぶ    Aで決定", 24);
    hintText_->position_ = { kScreenWidth * 0.5f, 640.0f };
    hintText_->scale_ = { 28.0f, 28.0f };
    hintText_->anchorPoint_ = { 0.5f, 0.5f };

    // 横に並べた3枚のカード
    float totalWidth = kCardWidth * static_cast<float>(kCardCount) + kCardGap * static_cast<float>(kCardCount - 1);
    float firstCenterX = (kScreenWidth - totalWidth) * 0.5f + kCardWidth * 0.5f;
    for (int32_t i = 0; i < kCardCount; ++i) {
        size_t index = static_cast<size_t>(i);
        UpgradeType type = static_cast<UpgradeType>(i);
        float centerX = firstCenterX + (kCardWidth + kCardGap) * static_cast<float>(i);

        cardSprites_[index] = std::make_unique<Sprite>(Vector2{ centerX, kCardCenterY }, Vector2{ kCardWidth, kCardHeight }, Vector2{ 0.5f, 0.5f }, Vector4{ 0.1f, 0.15f, 0.2f, 0.9f });
        cardSprites_[index]->textureHandle_ = whiteTextureHandle;

        nameTexts_[index] = std::make_unique<Text>(font, PlayerUpgrade::GetName(type), 8);
        nameTexts_[index]->position_ = { centerX, kCardCenterY - 100.0f };
        nameTexts_[index]->scale_ = { 48.0f, 48.0f };
        nameTexts_[index]->anchorPoint_ = { 0.5f, 0.5f };

        descriptionTexts_[index] = std::make_unique<Text>(font, "", 24);
        descriptionTexts_[index]->position_ = { centerX, kCardCenterY };
        descriptionTexts_[index]->scale_ = { 26.0f, 26.0f };
        descriptionTexts_[index]->anchorPoint_ = { 0.5f, 0.5f };

        countTexts_[index] = std::make_unique<Text>(font, "", 16);
        countTexts_[index]->position_ = { centerX, kCardCenterY + 120.0f };
        countTexts_[index]->scale_ = { 28.0f, 28.0f };
        countTexts_[index]->anchorPoint_ = { 0.5f, 0.5f };
    }

    RefreshTexts();
}

void UpgradeSelectUI::Open() {
    isOpen_ = true;
    cursor_ = 0;
    openTimer_ = 0.0f;

    // 開いた時点でAを押していても、離すまでは決定しない。左右も一度中立に戻すまで動かさない
    needReleaseA_ = true;
    needNeutral_ = true;

    RefreshTexts();
    UpdateCardViews();
}

void UpgradeSelectUI::Update() {
    debugParam_->ApplyIfDirty();
    if (!isOpen_) {
        return;
    }

    // 動きは止まらない時間で進める（ゲームの時間は止まっている）
    float deltaTime = FpsCounter::GetDeltaTime();

    UpdateCursor();
    UpdateConfirm(deltaTime);

    // 決定でこの画面が閉じた（または開き直した）場合も、現在の状態を表示用に整える
    UpdateCardViews();
}

void UpgradeSelectUI::UpdateCursor() {
    bool isLeft = inputCommand_->IsCommandActive("MoveLeft");
    bool isRight = inputCommand_->IsCommandActive("MoveRight");

    // 左右のどちらも倒していなければ中立。中立に戻すまで次の移動をしない
    if (!isLeft && !isRight) {
        needNeutral_ = false;
        return;
    }
    if (needNeutral_ || (isLeft && isRight)) {
        return;
    }

    if (isLeft) {
        cursor_ = (std::max)(cursor_ - 1, 0);
    } else {
        cursor_ = (std::min)(cursor_ + 1, kCardCount - 1);
    }
    needNeutral_ = true;
}

void UpgradeSelectUI::UpdateConfirm(float deltaTime) {
    openTimer_ += deltaTime;

    // 開いた時点でAを押していた場合は、一度離すまで受け付けない
    if (needReleaseA_ && !inputCommand_->IsCommandActive(kHoldCommand)) {
        needReleaseA_ = false;
    }
    if (needReleaseA_ || openTimer_ < confirmLockTime_) {
        return;
    }
    if (!inputCommand_->IsCommandActive(kConfirmCommand)) {
        return;
    }

    // 先に閉じてから通知する。通知の中で次の画面を開いてもよい
    isOpen_ = false;
    if (selectedCallback_) {
        selectedCallback_(static_cast<UpgradeType>(cursor_));
    }
}

void UpgradeSelectUI::RefreshTexts() {
    for (int32_t i = 0; i < kCardCount; ++i) {
        size_t index = static_cast<size_t>(i);
        UpgradeType type = static_cast<UpgradeType>(i);
        descriptionTexts_[index]->SetText(upgrade_->GetDescription(type));
        countTexts_[index]->SetText("取得 " + std::to_string(upgrade_->GetCount(type)) + " 回");
    }
}

void UpgradeSelectUI::UpdateCardViews() {
    for (int32_t i = 0; i < kCardCount; ++i) {
        size_t index = static_cast<size_t>(i);

        // 選んでいるカードは明るく、少し大きくする
        bool isSelected = (i == cursor_);
        cardSprites_[index]->color_ = isSelected ? Vector4{ 0.25f, 0.55f, 0.75f, 0.95f } : Vector4{ 0.1f, 0.15f, 0.2f, 0.9f };
        float scale = isSelected ? 1.08f : 1.0f;
        cardSprites_[index]->scale_ = { scale, scale };
        cardSprites_[index]->Update();

        nameTexts_[index]->Update();
        descriptionTexts_[index]->Update();
        countTexts_[index]->Update();
    }

    dimSprite_->Update();
    titleText_->Update();
    hintText_->Update();
}

void UpgradeSelectUI::Draw() {
    if (!isOpen_) {
        return;
    }

    // 暗い板、カード、文字の順に重ねる
    renderQueue_->SubmitSprite(dimSprite_.get());
    for (std::unique_ptr<Sprite>& card : cardSprites_) {
        renderQueue_->SubmitSprite(card.get());
    }
    renderQueue_->SubmitText(titleText_.get());
    renderQueue_->SubmitText(hintText_.get());
    for (int32_t i = 0; i < kCardCount; ++i) {
        size_t index = static_cast<size_t>(i);
        renderQueue_->SubmitText(nameTexts_[index].get());
        renderQueue_->SubmitText(descriptionTexts_[index].get());
        renderQueue_->SubmitText(countTexts_[index].get());
    }
}

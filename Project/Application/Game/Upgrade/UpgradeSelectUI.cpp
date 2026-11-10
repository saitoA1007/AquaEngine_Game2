#include "pch.h"
#include "UpgradeSelectUI.h"
#include "FPSCounter.h"
#include <algorithm>
#include <stdexcept>
#include <string>
using namespace GameEngine;

UpgradeSelectUI::UpgradeSelectUI(InputCommand* inputCommand, FontManager* fontManager, const UpgradeSelectTextures& textures, const PlayerUpgrade* upgrade)
    : inputCommand_(inputCommand), upgrade_(upgrade) {

    // フォントが見つからなければ例外にする
    const Font* font = fontManager->GetFont(kFontName);
    if (font == nullptr) {
        throw std::runtime_error(std::string("font not found: ") + kFontName);
    }

    RegisterDebugParameters();
    CreateHeaderViews(font, textures);
    CreateCards(font, textures);
    RefreshTexts();
}

void UpgradeSelectUI::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("UpgradeSelectUI");
    RegisterMotionParameters();
    RegisterShardParameters();
    RegisterColorParameters();
    debugParam_->Apply();
}

void UpgradeSelectUI::RegisterMotionParameters() {
    const std::string kMotionGroup = "Motion";

    debugParam_->Register("assembleTime", style_.assembleTime, 0, kMotionGroup);
    debugParam_->Register("cardStagger", style_.cardStagger, 1, kMotionGroup);
    debugParam_->Register("scatterDistance", style_.scatterDistance, 2, kMotionGroup);
    debugParam_->Register("scatterRotation", style_.scatterRotation, 3, kMotionGroup);
    debugParam_->Register("textFadeTime", style_.textFadeTime, 4, kMotionGroup);
    debugParam_->Register("textRiseDistance", style_.textRiseDistance, 5, kMotionGroup);
    debugParam_->Register("selectedScale", style_.selectedScale, 6, kMotionGroup);
    debugParam_->Register("lightPeriod", style_.lightPeriod, 7, kMotionGroup);
}

void UpgradeSelectUI::RegisterShardParameters() {
    const std::string kShardGroup = "Shard";

    debugParam_->Register("shardAngle", style_.shardAngle, 0, kShardGroup);
    debugParam_->Register("shardColumns", style_.shardColumns, 1, kShardGroup);
    debugParam_->Register("shardRows", style_.shardRows, 2, kShardGroup);
    debugParam_->Register("shardJitter", style_.shardJitter, 3, kShardGroup);
}

void UpgradeSelectUI::RegisterColorParameters() {
    const std::string kColorGroup = "Color";

    debugParam_->Register("plateColor", style_.plateColor, 0, kColorGroup);
    debugParam_->Register("plateSelectedColor", style_.plateSelectedColor, 1, kColorGroup);
    debugParam_->Register("frameColor", style_.frameColor, 2, kColorGroup);
    debugParam_->Register("frameSelectedColor", style_.frameSelectedColor, 3, kColorGroup);
    debugParam_->Register("lightColor", style_.lightColor, 4, kColorGroup);
    debugParam_->Register("dividerColor", style_.dividerColor, 5, kColorGroup);
    debugParam_->Register("textColor", style_.textColor, 6, kColorGroup);
    debugParam_->Register("shadowColor", style_.shadowColor, 7, kColorGroup);
    debugParam_->Register("shadowOffset", style_.shadowOffset, 8, kColorGroup);
}

void UpgradeSelectUI::CreateHeaderViews(const Font* font, const UpgradeSelectTextures& textures) {
    // 画面全体を覆う暗い板
    dimSprite_ = std::make_unique<Sprite>(Vector2{ 0.0f, 0.0f }, Vector2{ kScreenWidth, kScreenHeight }, Vector2{ 0.0f, 0.0f }, Vector4{ 0.0f, 0.0f, 0.0f, 0.65f });
    dimSprite_->textureHandle_ = textures.white;

    // 見出しの下の区切り線（画像は2倍の解像度で、表示は640x32）
    dividerSprite_ = std::make_unique<Sprite>(Vector2{ kScreenWidth * 0.5f, kDividerY }, Vector2{ 640.0f, 32.0f }, Vector2{ 0.5f, 0.5f }, style_.dividerColor);
    dividerSprite_->textureHandle_ = textures.divider;

    // 見出しと操作の案内
    titleText_ = std::make_unique<ShadowedText>(font, "強化を選んでください", 16, 44.0f);
    hintText_ = std::make_unique<ShadowedText>(font, "左右で選ぶ    Aで決定", 24, 28.0f);
}

void UpgradeSelectUI::CreateCards(const Font* font, const UpgradeSelectTextures& textures) {
    float totalWidth = kCardWidth * static_cast<float>(kCardCount) + kCardGap * static_cast<float>(kCardCount - 1);
    float firstCenterX = (kScreenWidth - totalWidth) * 0.5f + kCardWidth * 0.5f;
    for (int32_t i = 0; i < kCardCount; ++i) {
        size_t index = static_cast<size_t>(i);
        UpgradeType type = static_cast<UpgradeType>(i);
        float centerX = firstCenterX + (kCardWidth + kCardGap) * static_cast<float>(i);
        cards_[index] = std::make_unique<UpgradeCardView>(font, textures, &style_, Vector2{ centerX, kCardCenterY }, PlayerUpgrade::GetName(type));
    }
}

bool UpgradeSelectUI::IsAssembled() const {
    return openTimer_ >= style_.assembleTime;
}

void UpgradeSelectUI::Open() {
    isOpen_ = true;
    cursor_ = 0;
    openTimer_ = 0.0f;

    // 開いた時点でAを押していても、離すまでは決定しない。左右も一度中立に戻すまで動かさない
    needReleaseA_ = true;
    needNeutral_ = true;

    // カードは左から順に、少しずつ遅れて組み上がる。最後のカードが、全体の長さで組み上がる
    float staggerTotal = style_.cardStagger * static_cast<float>(kCardCount - 1);
    float duration = style_.assembleTime - staggerTotal;
    for (int32_t i = 0; i < kCardCount; ++i) {
        cards_[static_cast<size_t>(i)]->Start(style_.cardStagger * static_cast<float>(i), duration);
    }

    RefreshTexts();
    UpdateViews(0.0f);
}

void UpgradeSelectUI::Update() {
    debugParam_->ApplyIfDirty();
    if (!isOpen_) {
        return;
    }

    // 動きは止まらない時間で進める（ゲームの時間は止まっている）
    float deltaTime = FpsCounter::GetDeltaTime();
    openTimer_ += deltaTime;

    UpdateCursor();
    UpdateConfirm();

    // 決定でこの画面が閉じた（または開き直した）場合も、現在の状態を表示用に整える
    UpdateViews(deltaTime);
}

void UpgradeSelectUI::UpdateCursor() {
    bool isLeft = inputCommand_->IsCommandActive("MoveLeft");
    bool isRight = inputCommand_->IsCommandActive("MoveRight");

    // 左右のどちらも倒していなければ中立。中立に戻すまで次の移動をしない
    if (!isLeft && !isRight) {
        needNeutral_ = false;
        return;
    }

    // 組み上がるまではカーソルを動かさない
    if (!IsAssembled() || needNeutral_ || (isLeft && isRight)) {
        return;
    }

    if (isLeft) {
        cursor_ = (std::max)(cursor_ - 1, 0);
    } else {
        cursor_ = (std::min)(cursor_ + 1, kCardCount - 1);
    }
    needNeutral_ = true;
}

void UpgradeSelectUI::UpdateConfirm() {
    // 開いた時点でAを押していた場合は、一度離すまで受け付けない
    if (needReleaseA_ && !inputCommand_->IsCommandActive(kHoldCommand)) {
        needReleaseA_ = false;
    }

    // 組み上がるまでは決定を受け付けない
    if (needReleaseA_ || !IsAssembled()) {
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
        UpgradeType type = static_cast<UpgradeType>(i);
        cards_[static_cast<size_t>(i)]->SetTexts(upgrade_->GetDescription(type), "取得 " + std::to_string(upgrade_->GetCount(type)) + " 回");
    }
}

void UpgradeSelectUI::UpdateViews(float deltaTime) {
    for (int32_t i = 0; i < kCardCount; ++i) {
        cards_[static_cast<size_t>(i)]->Update(deltaTime, i == cursor_);
    }

    // 見出しと案内は、開いた時点からそのまま出す
    dimSprite_->Update();
    dividerSprite_->color_ = style_.dividerColor;
    dividerSprite_->Update();
    titleText_->Update({ kScreenWidth * 0.5f, kTitleY }, style_, 1.0f);
    hintText_->Update({ kScreenWidth * 0.5f, kHintY }, style_, 1.0f);
}

void UpgradeSelectUI::Draw() {
    if (!isOpen_) {
        return;
    }

    // 暗い板、区切り線、カードの順に重ねてから、文字を重ねる
    renderQueue_->SubmitSprite(dimSprite_.get());
    renderQueue_->SubmitSprite(dividerSprite_.get());
    for (std::unique_ptr<UpgradeCardView>& card : cards_) {
        card->DrawSprites(renderQueue_);
    }
    titleText_->Draw(renderQueue_);
    hintText_->Draw(renderQueue_);
    for (std::unique_ptr<UpgradeCardView>& card : cards_) {
        card->DrawTexts(renderQueue_);
    }
}

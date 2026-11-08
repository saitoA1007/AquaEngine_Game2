#include "pch.h"
#include "GameHud.h"
#include "FPSCounter.h"
#include <stdexcept>
#include <string>
using namespace GameEngine;

GameHud::GameHud(FontManager* fontManager, uint32_t circleTextureHandle) {
    // フォントが見つからなければ例外にする
    const Font* font = fontManager->GetFont(kFontName);
    if (font == nullptr) {
        throw std::runtime_error(std::string("font not found: ") + kFontName);
    }

    RegisterDebugParameters();
    hpIconView_ = std::make_unique<HpIconView>(circleTextureHandle);
    CreateViews(font);
}

void GameHud::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("GameHud");
    debugParam_->Register("phaseTextPosition", phaseTextPosition_, 0);
    debugParam_->Register("phaseTextSize", phaseTextSize_, 1);
    debugParam_->Register("textColor", textColor_, 2);
    debugParam_->Apply();
}

void GameHud::CreateViews(const Font* font) {
    // フェーズの文字は右上を基準にする
    phaseText_ = std::make_unique<Text>(font, "PHASE 1", 12);
    phaseText_->anchorPoint_ = { 1.0f, 0.0f };
}

void GameHud::SetInput(const HudInput& input) {
    input_ = input;
    hpIconView_->SetHp(input_.hp, input_.maxHp);
}

void GameHud::Update() {
    debugParam_->ApplyIfDirty();
    hpIconView_->Update(FpsCounter::GetDeltaTime());
}

void GameHud::RefreshTexts() {
    // 値が変わったときだけ文字を作り直す
    if (shownPhase_ != input_.phase) {
        shownPhase_ = input_.phase;
        phaseText_->SetText("PHASE " + std::to_string(input_.phase));
    }
}

void GameHud::UpdateViews() {
    RefreshTexts();
    phaseText_->position_ = phaseTextPosition_;
    phaseText_->scale_ = { phaseTextSize_, phaseTextSize_ };
    phaseText_->color_ = textColor_;
    phaseText_->Update();
}

void GameHud::Draw() {
    // 表示する値はシーンの更新の最後に届くため、描画の直前に反映して1フレームの遅れをなくす
    UpdateViews();

    hpIconView_->Draw(renderQueue_);
    renderQueue_->SubmitText(phaseText_.get());
}

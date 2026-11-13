#include "pch.h"
#include "OutlinedText.h"
#include <cmath>
#include <numbers>
using namespace GameEngine;

OutlinedText::OutlinedText(const Font* font, const std::string& text, uint32_t maxLength) {
    shadow_ = std::make_unique<Text>(font, text, maxLength);
    for (std::unique_ptr<Text>& outline : outlines_) {
        outline = std::make_unique<Text>(font, text, maxLength);
    }
    body_ = std::make_unique<Text>(font, text, maxLength);
}

void OutlinedText::SetText(const std::string& text) {
    shadow_->SetText(text);
    for (std::unique_ptr<Text>& outline : outlines_) {
        outline->SetText(text);
    }
    body_->SetText(text);
}

void OutlinedText::SetFontSize(float fontSize) {
    fontSize_ = fontSize;
    body_->scale_ = { fontSize, fontSize };
}

void OutlinedText::SetAnchor(const Vector2& anchor) {
    shadow_->anchorPoint_ = anchor;
    for (std::unique_ptr<Text>& outline : outlines_) {
        outline->anchorPoint_ = anchor;
    }
    body_->anchorPoint_ = anchor;
}

void OutlinedText::Update(const Vector2& position, const Style& style, float alpha) {
    Vector2 scale = { fontSize_, fontSize_ };

    // 影: 右下へずらす
    shadow_->scale_ = scale;
    shadow_->position_ = { position.x + style.shadowRatio.x * fontSize_, position.y + style.shadowRatio.y * fontSize_ };
    shadow_->color_ = { style.shadowColor.x, style.shadowColor.y, style.shadowColor.z, style.shadowColor.w * alpha };
    shadow_->Update();

    // 縁: 円周上の8方向へずらす（斜めも同じ距離にして、角ばらないようにする）
    float outlinePx = style.outlineRatio * fontSize_;
    for (int32_t i = 0; i < kOutlineCount; ++i) {
        float angle = std::numbers::pi_v<float> * 2.0f * static_cast<float>(i) / static_cast<float>(kOutlineCount);
        Text* outline = outlines_[static_cast<size_t>(i)].get();
        outline->scale_ = scale;
        outline->position_ = { position.x + std::cos(angle) * outlinePx, position.y + std::sin(angle) * outlinePx };
        outline->color_ = { style.outlineColor.x, style.outlineColor.y, style.outlineColor.z, style.outlineColor.w * alpha };
        outline->Update();
    }

    // 本体
    body_->scale_ = scale;
    body_->position_ = position;
    body_->color_ = { style.color.x, style.color.y, style.color.z, style.color.w * alpha };
    body_->Update();
}

void OutlinedText::Submit(RenderQueue* renderQueue) {
    renderQueue->SubmitText(shadow_.get());
    for (std::unique_ptr<Text>& outline : outlines_) {
        renderQueue->SubmitText(outline.get());
    }
    renderQueue->SubmitText(body_.get());
}

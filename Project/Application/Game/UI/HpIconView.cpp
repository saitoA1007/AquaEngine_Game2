#include "pch.h"
#include "HpIconView.h"
#include <algorithm>
#include <cmath>
#include <numbers>
using namespace GameEngine;

HpIconView::HpIconView(uint32_t textureHandle) : textureHandle_(textureHandle) {
    RegisterDebugParameters();

    // 演出がない状態から始める
    popTimer_ = popTime_;
}

void HpIconView::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>(kDebugName);
    debugParam_->Register("origin", origin_, 0);
    debugParam_->Register("iconSize", iconSize_, 1);
    debugParam_->Register("spacing", spacing_, 2);
    debugParam_->Register("aliveColor", aliveColor_, 3);
    debugParam_->Register("lostColor", lostColor_, 4);
    debugParam_->Register("flashColor", flashColor_, 5);
    debugParam_->Register("popScale", popScale_, 6);
    debugParam_->Register("popTime", popTime_, 7);
    debugParam_->Register("popPeakRatio", popPeakRatio_, 8);
    debugParam_->Apply();
}

void HpIconView::ResizeIcons(int32_t count) {
    // 足りない分は作り、多い分は減らす
    size_t target = static_cast<size_t>((std::max)(count, 0));
    while (icons_.size() < target) {
        std::unique_ptr<Sprite> icon = std::make_unique<Sprite>(Vector2{ 0.0f, 0.0f }, Vector2{ iconSize_, iconSize_ }, Vector2{ 0.5f, 0.5f }, aliveColor_);
        icon->textureHandle_ = textureHandle_;
        icons_.push_back(std::move(icon));
    }
    if (icons_.size() > target) {
        icons_.resize(target);
    }
}

void HpIconView::SetHp(int32_t hp, int32_t maxHp) {
    ResizeIcons(maxHp);

    // 被弾の通知があり、HPが減っていれば、減った分のアイコンの演出を始める。初回は演出しない
    if (isDamagePending_ && shownHp_ >= 0 && hp < shownHp_) {
        popFrom_ = (std::max)(hp, 0);
        popTo_ = shownHp_;
        popTimer_ = 0.0f;
    }
    isDamagePending_ = false;

    hp_ = hp;
    shownHp_ = hp;
}

void HpIconView::Update(float deltaTime) {
    debugParam_->ApplyIfDirty();
    popTimer_ += deltaTime;
}

void HpIconView::GetIconLook(int32_t index, float& scale, Vector4& color) const {
    scale = 1.0f;

    // 残りのアイコンはそのまま
    if (index < hp_) {
        color = aliveColor_;
        return;
    }

    // 演出の対象でなければ、暗い色のまま
    bool isPopping = (index >= popFrom_ && index < popTo_) && popTimer_ < popTime_;
    if (!isPopping) {
        color = lostColor_;
        return;
    }

    // 最大の大きさまで素早く弾け、そこから元の大きさへ戻りながら、白から暗い色へ変わる
    float t = (std::min)(popTimer_ / (std::max)(popTime_, 0.01f), 1.0f);
    float peak = (std::clamp)(popPeakRatio_, 0.01f, 0.99f);
    float pulse = 0.0f;
    if (t < peak) {
        pulse = std::sin((t / peak) * std::numbers::pi_v<float> * 0.5f);
    } else {
        pulse = 1.0f - (t - peak) / (1.0f - peak);
    }
    scale = 1.0f + (popScale_ - 1.0f) * pulse;

    color = {
        flashColor_.x + (lostColor_.x - flashColor_.x) * t,
        flashColor_.y + (lostColor_.y - flashColor_.y) * t,
        flashColor_.z + (lostColor_.z - flashColor_.z) * t,
        flashColor_.w + (lostColor_.w - flashColor_.w) * t };
}

void HpIconView::Draw(RenderQueue* renderQueue) {
    for (size_t i = 0; i < icons_.size(); ++i) {
        // アンカーを中心にして、弾けるときは中心から大きくなる
        float centerX = origin_.x + iconSize_ * 0.5f + static_cast<float>(i) * (iconSize_ + spacing_);
        float centerY = origin_.y + iconSize_ * 0.5f;
        icons_[i]->position_ = { centerX, centerY };
        icons_[i]->size_ = { iconSize_, iconSize_ };

        float scale = 1.0f;
        Vector4 color = aliveColor_;
        GetIconLook(static_cast<int32_t>(i), scale, color);
        icons_[i]->scale_ = { scale, scale };
        icons_[i]->color_ = color;
        icons_[i]->Update();

        renderQueue->SubmitSprite(icons_[i].get());
    }
}

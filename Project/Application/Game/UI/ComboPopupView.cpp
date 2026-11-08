#include "pch.h"
#include "ComboPopupView.h"
#include "FPSCounter.h"
#include <algorithm>
#include <stdexcept>
#include <string>
using namespace GameEngine;

ComboPopupView::ComboPopupView(const Camera* camera, FontManager* fontManager) : camera_(camera) {
    if (camera_ == nullptr) {
        throw std::runtime_error("ComboPopupView: camera is null");
    }

    // フォントが見つからなければ例外にする
    const Font* font = fontManager->GetFont(kFontName);
    if (font == nullptr) {
        throw std::runtime_error(std::string("font not found: ") + kFontName);
    }

    RegisterDebugParameters();

    // 文字は上限の数だけ作って使い回す
    for (Popup& popup : popups_) {
        popup.text = std::make_unique<Text>(font, "1 HIT", 12);
        popup.text->anchorPoint_ = { 0.5f, 0.5f };
    }
}

void ComboPopupView::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("ComboPopupView");
    debugParam_->Register("maxPopups", maxPopups_, 0);
    debugParam_->Register("holdTime", holdTime_, 1);
    debugParam_->Register("fadeTime", fadeTime_, 2);
    debugParam_->Register("baseSize", baseSize_, 3);
    debugParam_->Register("growPerHit", growPerHit_, 4);
    debugParam_->Register("maxSize", maxSize_, 5);
    debugParam_->Register("punchScale", punchScale_, 6);
    debugParam_->Register("punchTime", punchTime_, 7);
    debugParam_->Register("offset", offset_, 8);
    debugParam_->Register("color", color_, 9);
    debugParam_->Register("screenMargin", screenMargin_, 10);
    debugParam_->Apply();
}

Vector2 ComboPopupView::WorldToScreen(const Vector3& worldPosition) const {
    // ビュー射影行列でクリップ空間へ変換し、-1〜1を画面の大きさへ直す（Yは上が正なので反転する）
    Vector3 ndc = Matrix4x4::Transform(worldPosition, camera_->GetVPMatrix());
    return { (ndc.x * 0.5f + 0.5f) * kScreenWidth, (1.0f - (ndc.y * 0.5f + 0.5f)) * kScreenHeight };
}

void ComboPopupView::AddHit(uint32_t chainId, const Vector3& worldPosition) {
    // 画面の端で見切れないよう、文字の中心を画面の内側へ収める
    Vector2 screen = WorldToScreen(worldPosition);
    Vector2 position = {
        (std::clamp)(screen.x + offset_.x, screenMargin_, kScreenWidth - screenMargin_),
        (std::clamp)(screen.y + offset_.y, screenMargin_, kScreenHeight - screenMargin_) };

    // 同じ連鎖の表示があれば、数を増やして新しい位置へ移す
    int32_t limit = (std::clamp)(maxPopups_, 0, kMaxPopups);
    for (int32_t i = 0; i < limit; ++i) {
        Popup& popup = popups_[static_cast<size_t>(i)];
        if (!popup.isActive || popup.chainId != chainId) {
            continue;
        }
        ++popup.count;
        popup.position = position;
        popup.idleTimer = 0.0f;
        popup.punchTimer = 0.0f;
        popup.text->SetText(std::to_string(popup.count) + " HIT");
        return;
    }

    // なければ空きの表示を使い、「1 HIT」から出す。空きがなければ出さない
    for (int32_t i = 0; i < limit; ++i) {
        Popup& popup = popups_[static_cast<size_t>(i)];
        if (popup.isActive) {
            continue;
        }
        popup.isActive = true;
        popup.chainId = chainId;
        popup.count = 1;
        popup.position = position;
        popup.idleTimer = 0.0f;
        popup.punchTimer = 0.0f;
        popup.text->SetText("1 HIT");
        return;
    }
}

void ComboPopupView::Update() {
    debugParam_->ApplyIfDirty();

    float deltaTime = FpsCounter::GetGameDeltaTime();
    for (Popup& popup : popups_) {
        if (popup.isActive) {
            UpdatePopup(popup, deltaTime);
        }
    }
}

void ComboPopupView::UpdatePopup(Popup& popup, float deltaTime) {
    popup.idleTimer += deltaTime;
    popup.punchTimer += deltaTime;

    // 増えなくなって、消える時間も過ぎたら、表示を終える
    if (popup.idleTimer >= holdTime_ + fadeTime_) {
        popup.isActive = false;
        return;
    }

    // 数が大きいほど大きく。増えた瞬間は大きくなり、すぐ戻る
    float size = (std::min)(baseSize_ + growPerHit_ * static_cast<float>(popup.count - 1), maxSize_);
    float punchT = (std::min)(popup.punchTimer / (std::max)(punchTime_, 0.01f), 1.0f);
    size *= 1.0f + (punchScale_ - 1.0f) * (1.0f - punchT);

    // 消え始めてから、透明にしていく
    float alpha = 1.0f;
    if (popup.idleTimer > holdTime_) {
        alpha = 1.0f - (popup.idleTimer - holdTime_) / (std::max)(fadeTime_, 0.01f);
    }

    popup.text->position_ = popup.position;
    popup.text->scale_ = { size, size };
    popup.text->color_ = { color_.x, color_.y, color_.z, color_.w * alpha };
    popup.text->Update();
}

void ComboPopupView::Draw() {
    for (Popup& popup : popups_) {
        if (!popup.isActive) {
            continue;
        }
        renderQueue_->SubmitText(popup.text.get());
    }
}

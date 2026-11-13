#include "pch.h"
#include "ComboPopupView.h"
#include "FPSCounter.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
using namespace GameEngine;

ComboPopupView::ComboPopupView(const Camera* camera, FontManager* fontManager, uint32_t burstTextureHandle) : camera_(camera) {
    if (camera_ == nullptr) {
        throw std::runtime_error("ComboPopupView: camera is null");
    }

    // フォントが見つからなければ例外にする
    const Font* font = fontManager->GetFont(kFontName);
    if (font == nullptr) {
        throw std::runtime_error(std::string("font not found: ") + kFontName);
    }

    RegisterDebugParameters();
    CreateViews(font, burstTextureHandle);
}

void ComboPopupView::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("ComboPopupView");
    RegisterSizeParameters();
    RegisterLayoutParameters();
    RegisterColorParameters();
    debugParam_->Apply();
}

void ComboPopupView::RegisterSizeParameters() {
    debugParam_->Register("maxPopups", maxPopups_, 0);
    debugParam_->Register("holdTime", holdTime_, 1);
    debugParam_->Register("fadeTime", fadeTime_, 2);
    debugParam_->Register("baseSize", baseSize_, 3);
    debugParam_->Register("growPerHit", growPerHit_, 4);
    debugParam_->Register("maxSize", maxSize_, 5);
    debugParam_->Register("punchScale", punchScale_, 6);
    debugParam_->Register("punchTime", punchTime_, 7);
}

void ComboPopupView::RegisterLayoutParameters() {
    debugParam_->Register("offset", offset_, 8);
    debugParam_->Register("hitSizeRatio", hitSizeRatio_, 9);
    debugParam_->Register("hitOffsetRatio", hitOffsetRatio_, 10);
    debugParam_->Register("burstSizeRatio", burstSizeRatio_, 11);
    debugParam_->Register("burstPunchScale", burstPunchScale_, 12);
    debugParam_->Register("burstAngleRange", burstAngleRange_, 13);
    debugParam_->Register("screenMargin", screenMargin_, 14);
}

void ComboPopupView::RegisterColorParameters() {
    // 色の名前は、意味が変わった旧「color」（黄色の1色）と区別するため付け直している
    debugParam_->Register("numberColor", numberColor_, 15);
    debugParam_->Register("hitColor", hitColor_, 16);
    debugParam_->Register("outlineColor", outlineColor_, 17);
    debugParam_->Register("shadowColor", shadowColor_, 18);
    debugParam_->Register("burstColor", burstColor_, 19);
    debugParam_->Register("outlineRatio", outlineRatio_, 20);
    debugParam_->Register("shadowRatio", shadowRatio_, 21);
}

void ComboPopupView::CreateViews(const Font* font, uint32_t burstTextureHandle) {
    // 表示は上限の数だけ作って使い回す
    for (Popup& popup : popups_) {
        popup.number = std::make_unique<OutlinedText>(font, "1", kNumberMaxLength);
        popup.number->SetAnchor({ 0.5f, 0.5f });

        // 「HIT!」は左下を基準にして、数字の右下へ置く
        popup.hit = std::make_unique<OutlinedText>(font, kHitText, kHitMaxLength);
        popup.hit->SetAnchor({ 0.0f, 1.0f });

        popup.burst = std::make_unique<Sprite>(Vector2{ 0.0f, 0.0f }, Vector2{ 1.0f, 1.0f }, Vector2{ 0.5f, 0.5f }, Vector4{ 1.0f, 1.0f, 1.0f, 0.0f });
        popup.burst->textureHandle_ = burstTextureHandle;
    }
}

Vector2 ComboPopupView::WorldToScreen(const Vector3& worldPosition) const {
    // ビュー射影行列でクリップ空間へ変換し、-1〜1を画面の大きさへ直す（Yは上が正なので反転する）
    Vector3 ndc = Matrix4x4::Transform(worldPosition, camera_->GetVPMatrix());
    return { (ndc.x * 0.5f + 0.5f) * kScreenWidth, (1.0f - (ndc.y * 0.5f + 0.5f)) * kScreenHeight };
}

void ComboPopupView::StartPunch(Popup& popup) {
    popup.punchTimer = 0.0f;

    // 破裂の向きを、今の向きから少しだけ別の向きへ変える（弾みの間に回る）
    std::uniform_real_distribution<float> distribution(-burstAngleRange_, burstAngleRange_);
    popup.burstFromAngle = popup.burstToAngle;
    popup.burstToAngle = popup.burstFromAngle + distribution(random_);
}

void ComboPopupView::AddHit(uint32_t chainId, const Vector3& worldPosition) {
    // 氷霧で倒した撃破（連鎖IDが0）は数えない。氷塊で倒したときだけ数える
    if (chainId == 0) {
        return;
    }

    // 数字の中心にしたい位置。画面の内側へ寄せるのは、大きさが決まる毎フレームの更新で行う
    Vector2 screen = WorldToScreen(worldPosition);
    Vector2 target = { screen.x + offset_.x, screen.y + offset_.y };

    // 同じ連鎖の表示があれば、数を増やして新しい位置へ移す
    int32_t limit = (std::clamp)(maxPopups_, 0, kMaxPopups);
    for (int32_t i = 0; i < limit; ++i) {
        Popup& popup = popups_[static_cast<size_t>(i)];
        if (!popup.isActive || popup.chainId != chainId) {
            continue;
        }
        ++popup.count;
        popup.targetPosition = target;
        popup.idleTimer = 0.0f;
        popup.number->SetText(std::to_string(popup.count));
        StartPunch(popup);
        UpdatePopup(popup, 0.0f);
        return;
    }

    // なければ空きの表示を使い、1から出す。空きがなければ出さない
    for (int32_t i = 0; i < limit; ++i) {
        Popup& popup = popups_[static_cast<size_t>(i)];
        if (popup.isActive) {
            continue;
        }
        popup.isActive = true;
        popup.chainId = chainId;
        popup.count = 1;
        popup.targetPosition = target;
        popup.idleTimer = 0.0f;
        popup.number->SetText("1");
        StartPunch(popup);
        UpdatePopup(popup, 0.0f);
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

float ComboPopupView::CalcNumberSize(int32_t count, float punchRate) const {
    // 数が大きいほど大きく。増えた瞬間は大きくなり、すぐ戻る
    float size = (std::min)(baseSize_ + growPerHit_ * static_cast<float>(count - 1), maxSize_);
    return size * (1.0f + (punchScale_ - 1.0f) * (1.0f - punchRate));
}

Vector2 ComboPopupView::CalcHitOrigin(float numberSize, const Vector2& numberExtent) const {
    // 数字の右下の角から、数字の大きさに応じてずらす
    return { numberExtent.x * 0.5f + hitOffsetRatio_.x * numberSize, numberExtent.y * 0.5f + hitOffsetRatio_.y * numberSize };
}

void ComboPopupView::Bounds::Include(const Vector2& rectMin, const Vector2& rectMax) {
    // 最初の矩形は、そのまま外形にする
    if (isEmpty) {
        min = rectMin;
        max = rectMax;
        isEmpty = false;
        return;
    }
    min = { (std::min)(min.x, rectMin.x), (std::min)(min.y, rectMin.y) };
    max = { (std::max)(max.x, rectMax.x), (std::max)(max.y, rectMax.y) };
}

void ComboPopupView::IncludeText(Bounds& bounds, const Vector2& rectMin, const Vector2& rectMax, float fontSize) const {
    // 縁の分だけ広げた矩形と、影の分だけずらした矩形を含める
    float outline = outlineRatio_ * fontSize;
    Vector2 shadow = { shadowRatio_.x * fontSize, shadowRatio_.y * fontSize };
    bounds.Include({ rectMin.x - outline, rectMin.y - outline }, { rectMax.x + outline, rectMax.y + outline });
    bounds.Include({ rectMin.x + shadow.x, rectMin.y + shadow.y }, { rectMax.x + shadow.x, rectMax.y + shadow.y });
}

ComboPopupView::Bounds ComboPopupView::MeasureBounds(const Popup& popup, const Vector2& hitOrigin, float burstSize, float burstAngle) const {
    Bounds bounds;

    // 数字（中心が原点）
    Vector2 numberExtent = popup.number->GetSize();
    Vector2 numberHalf = { numberExtent.x * 0.5f, numberExtent.y * 0.5f };
    IncludeText(bounds, { -numberHalf.x, -numberHalf.y }, numberHalf, popup.number->GetFontSize());

    // 「HIT!」（左下が基準）
    Vector2 hitExtent = popup.hit->GetSize();
    IncludeText(bounds, { hitOrigin.x, hitOrigin.y - hitExtent.y }, { hitOrigin.x + hitExtent.x, hitOrigin.y }, popup.hit->GetFontSize());

    // 破裂（回った正方形が収まる範囲）
    float burstHalf = burstSize * 0.5f * (std::abs(std::cos(burstAngle)) + std::abs(std::sin(burstAngle)));
    bounds.Include({ -burstHalf, -burstHalf }, { burstHalf, burstHalf });

    return bounds;
}

float ComboPopupView::FitAxis(float value, float boundsMin, float boundsMax, float screenSize) const {
    // 外形が縁の内側に収まる、中心の範囲。範囲がないときは中央に揃える
    float low = screenMargin_ - boundsMin;
    float high = screenSize - screenMargin_ - boundsMax;
    if (low > high) {
        return (low + high) * 0.5f;
    }
    return (std::clamp)(value, low, high);
}

Vector2 ComboPopupView::FitInsideScreen(const Vector2& target, const Bounds& bounds) const {
    return { FitAxis(target.x, bounds.min.x, bounds.max.x, kScreenWidth), FitAxis(target.y, bounds.min.y, bounds.max.y, kScreenHeight) };
}

void ComboPopupView::PlaceViews(Popup& popup, const Vector2& center, const Vector2& hitOrigin, float burstSize, float burstAngle, float alpha) {
    // 数字
    OutlinedText::Style numberStyle;
    numberStyle.color = numberColor_;
    numberStyle.outlineColor = outlineColor_;
    numberStyle.shadowColor = shadowColor_;
    numberStyle.outlineRatio = outlineRatio_;
    numberStyle.shadowRatio = shadowRatio_;
    popup.number->Update(center, numberStyle, alpha);

    // 「HIT!」（縁と影は数字と同じ色・割合）
    OutlinedText::Style hitStyle = numberStyle;
    hitStyle.color = hitColor_;
    popup.hit->Update({ center.x + hitOrigin.x, center.y + hitOrigin.y }, hitStyle, alpha);

    // 破裂
    popup.burst->position_ = center;
    popup.burst->size_ = { burstSize, burstSize };
    popup.burst->rotate_ = burstAngle;
    popup.burst->color_ = { burstColor_.x, burstColor_.y, burstColor_.z, burstColor_.w * alpha };
    popup.burst->Update();
}

void ComboPopupView::UpdatePopup(Popup& popup, float deltaTime) {
    popup.idleTimer += deltaTime;
    popup.punchTimer += deltaTime;

    // 増えなくなって、消える時間も過ぎたら、表示を終える
    if (popup.idleTimer >= holdTime_ + fadeTime_) {
        popup.isActive = false;
        return;
    }

    // 消え始めてから、透明にしていく（破裂も一緒に消える）
    float alpha = 1.0f;
    if (popup.idleTimer > holdTime_) {
        alpha = 1.0f - (popup.idleTimer - holdTime_) / (std::max)(fadeTime_, 0.01f);
    }

    // 弾みの進み具合から、数字の大きさ、破裂の大きさと向きを決める
    float punchRate = (std::min)(popup.punchTimer / (std::max)(punchTime_, 0.01f), 1.0f);
    float numberSize = CalcNumberSize(popup.count, punchRate);
    float burstSize = numberSize * burstSizeRatio_ * (1.0f + (burstPunchScale_ - 1.0f) * (1.0f - punchRate));
    float turnRate = 1.0f - (1.0f - punchRate) * (1.0f - punchRate);
    float burstAngle = popup.burstFromAngle + (popup.burstToAngle - popup.burstFromAngle) * turnRate;

    // 文字の大きさを決めてから、表示全体の外形を測る
    popup.number->SetFontSize(numberSize);
    popup.hit->SetFontSize(numberSize * hitSizeRatio_);
    Vector2 hitOrigin = CalcHitOrigin(numberSize, popup.number->GetSize());
    Bounds bounds = MeasureBounds(popup, hitOrigin, burstSize, burstAngle);

    // 外形の全体が画面の内側に収まる位置へ寄せて、各表示へ反映する
    Vector2 center = FitInsideScreen(popup.targetPosition, bounds);
    PlaceViews(popup, center, hitOrigin, burstSize, burstAngle, alpha);
}

void ComboPopupView::Draw() {
    // 非表示の間は描かない（数と残り時間は保つ）
    if (!isVisible_) {
        return;
    }

    for (Popup& popup : popups_) {
        if (!popup.isActive) {
            continue;
        }
        // 破裂は文字より下に描かれる（エンジンの保証ではなく、現在の描画の順）
        renderQueue_->SubmitSprite(popup.burst.get());
        popup.hit->Submit(renderQueue_);
        popup.number->Submit(renderQueue_);
    }
}

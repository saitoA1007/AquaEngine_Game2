#include "pch.h"
#include "EnemyGaugeView.h"
#include "MyMath.h"
#include <algorithm>
#include <cmath>
using namespace GameEngine;

EnemyGaugeView::EnemyGaugeView(const Camera* camera, Model* planeModel, uint32_t whiteTextureHandle, uint32_t capacity)
    : camera_(camera), model_(planeModel), whiteTextureHandle_(whiteTextureHandle), capacity_(capacity) {

    candidates_.reserve(capacity_);

    // 白い材質。色は、インスタンスごとの色で決める
    material_.Initialize({ 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f, false);

    // インスタンスの領域を、敵の最大数だけ確保する
    Transform identity = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    backTransforms_.Initialize(capacity_, identity);
    fillTransforms_.Initialize(capacity_, identity);
}

void EnemyGaugeView::Draw(RenderQueue* renderQueue, const Settings& settings, const std::vector<Enemy*>& enemies, const Vector3& nearCenter) {
    if (!settings.isVisible || capacity_ == 0) {
        return;
    }

    // 凍りかけの敵だけを集める（凍った敵は殻が見えるので出さない）
    candidates_.clear();
    for (Enemy* enemy : enemies) {
        if (!enemy->IsFrozen() && enemy->GetIceRatio() > 0.0f) {
            candidates_.push_back(enemy);
        }
    }

    // 上限を超えるときは、基準位置に近い敵だけを残す
    size_t limit = static_cast<size_t>((std::clamp)(settings.maxCount, 0, static_cast<int32_t>(capacity_)));
    if (candidates_.size() > limit) {
        std::nth_element(candidates_.begin(), candidates_.begin() + limit, candidates_.end(),
            [&nearCenter](const Enemy* a, const Enemy* b) {
                float adx = a->GetPosition().x - nearCenter.x;
                float adz = a->GetPosition().z - nearCenter.z;
                float bdx = b->GetPosition().x - nearCenter.x;
                float bdz = b->GetPosition().z - nearCenter.z;
                return (adx * adx + adz * adz) < (bdx * bdx + bdz * bdz);
            });
        candidates_.resize(limit);
    }
    if (candidates_.empty()) {
        return;
    }

    // バーはカメラと同じ回転にして、常に画面と平行に見せる。カメラは固定角度（ヨーなし）で、画面の右はワールドのX軸
    // 平面は-Z向きなので、カメラの回転（X軸まわり）を掛けるとカメラの方を向く
    Vector3 rotate = camera_->transform_.rotate;
    Vector3 towardCamera = { 0.0f, std::sin(rotate.x), -std::cos(rotate.x) };

    for (size_t i = 0; i < candidates_.size(); ++i) {
        const Enemy* enemy = candidates_[i];
        float ratio = enemy->GetIceRatio();
        Vector3 head = { enemy->GetPosition().x, enemy->GetVisualScale() * 2.0f + settings.headOffset, enemy->GetPosition().z };

        // 背景は頭の真上。平面は一辺2なので、半分の大きさを拡大率にする
        WorldTransforms::TransformData& back = backTransforms_.transformDatas_[i];
        Vector3 backScale = { (settings.width + settings.border * 2.0f) * 0.5f, (settings.height + settings.border * 2.0f) * 0.5f, 1.0f };
        back.worldMatrix = Math::MakeWorldMatrixFromEulerRotation(head, rotate, backScale);
        back.color = settings.backColor;
        back.textureHandle = whiteTextureHandle_;

        // 塗りは左端をそろえて右へ伸ばし、背景よりカメラ側へ少し浮かせる
        WorldTransforms::TransformData& fill = fillTransforms_.transformDatas_[i];
        float fillWidth = settings.width * ratio;
        Vector3 fillPosition = {
            head.x - (settings.width - fillWidth) * 0.5f + towardCamera.x * kFillLift,
            head.y + towardCamera.y * kFillLift,
            head.z + towardCamera.z * kFillLift };
        Vector3 fillScale = { fillWidth * 0.5f, settings.height * 0.5f, 1.0f };
        fill.worldMatrix = Math::MakeWorldMatrixFromEulerRotation(fillPosition, rotate, fillScale);
        fill.color = settings.fillColor;
        fill.textureHandle = whiteTextureHandle_;
    }

    // 背景、塗りの順に、1回ずつの描画として登録する
    uint32_t count = static_cast<uint32_t>(candidates_.size());
    renderQueue->SubmitInstancing(model_, count, backTransforms_, 1.0f, BlendMode::kBlendModeNormal, &material_.GetMaterialBuffer());
    renderQueue->SubmitInstancing(model_, count, fillTransforms_, 1.0f, BlendMode::kBlendModeNormal, &material_.GetMaterialBuffer());
}

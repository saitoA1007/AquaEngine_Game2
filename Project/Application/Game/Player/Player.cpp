#include "pch.h"
#include "Player.h"
#include "FPSCounter.h"
#include <algorithm>
#include <cmath>
using namespace GameEngine;

Player::Player(Model* model, Model* discModel, InputCommand* inputCommand, const Field* field, const PlayerStats* stats)
    : inputCommand_(inputCommand), field_(field), stats_(stats), modelComponent_(model), iceAreaView_(discModel) {

    RegisterDebugParameters();

    modelComponent_.materialData_->enableLighting = 1;
}

void Player::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("Player");
    debugParam_->Register("radius", radius_);
    debugParam_->Register("color", color_);
    debugParam_->Register("iceAreaRadius", iceAreaRadius_);
    debugParam_->Register("iceAreaColor", iceAreaColor_);
    debugParam_->Register("initialHp", initialHp_);
    debugParam_->Register("invincibleTime", invincibleTime_);
    debugParam_->Apply();

    // 保存済みの初期HPを反映してから開始する
    hp_ = initialHp_;
}

void Player::Initialize() {
    modelComponent_.materialData_->color = color_;
}

Vector3 Player::GetMoveDirection() const {
    float x = 0.0f;
    float z = 0.0f;

    // スティックは倒し量を保つため、閾値を超えたときだけ値をそのまま使う
    Vector2 stick = inputCommand_->GetLeftStick();
    if (stick.Length() > kStickThreshold) {
        x = stick.x;
        z = stick.y;
    } else {
        // キーボードとDPadは真偽値のコマンドから方向を作る
        if (inputCommand_->IsCommandActive("MoveRight")) { x += 1.0f; }
        if (inputCommand_->IsCommandActive("MoveLeft")) { x -= 1.0f; }
        if (inputCommand_->IsCommandActive("MoveUp")) { z += 1.0f; }
        if (inputCommand_->IsCommandActive("MoveDown")) { z -= 1.0f; }
    }

    // 斜め入力で速くならないよう、長さを1以内に制限する
    float length = std::sqrt(x * x + z * z);
    if (length > 1.0f) {
        x /= length;
        z /= length;
    }
    return { x, 0.0f, z };
}

void Player::Update() {
    debugParam_->ApplyIfDirty();
    modelComponent_.materialData_->color = color_;

    // 無敵時間を進める
    invincibleTimer_ = (std::max)(invincibleTimer_ - FpsCounter::gameDeltaTime, 0.0f);

    // 加減速なしで、入力方向へ即座に移動する
    Vector3 direction = GetMoveDirection();
    position_ += direction * (stats_->moveSpeed * FpsCounter::gameDeltaTime);

    // フィールドの外へ出ないよう押し戻す
    position_ = field_->ClampToField(position_, radius_);

    // 球が地面に接するよう、半径に合わせて高さを決める
    position_.y = radius_;

    modelComponent_.worldTransform_.transform_.scale = { radius_, radius_, radius_ };
    modelComponent_.worldTransform_.transform_.translate = position_;
    modelComponent_.Update();
}

void Player::TakeDamage(int32_t damage) {
    // 無敵時間中、またはダメージがないときは何もしない
    if (IsInvincible() || damage <= 0) {
        return;
    }

    hp_ = (std::max)(hp_ - damage, 0);
    invincibleTimer_ = invincibleTime_;
}

void Player::ApplyPushback(const Vector3& offset) {
    // 押し戻したあともフィールド内に収める
    position_.x += offset.x;
    position_.z += offset.z;
    position_ = field_->ClampToField(position_, radius_);
    position_.y = radius_;

    // Updateの後に呼ばれるため、見た目の位置もここで更新する
    modelComponent_.worldTransform_.transform_.translate = position_;
    modelComponent_.Update();
}

void Player::Draw() {
    // 氷エリアはプレイヤーを中心に常に描く
    iceAreaView_.Draw(renderQueue_, position_, iceAreaRadius_, iceAreaColor_);
    modelComponent_.DrawRaytracing(renderQueue_);
}

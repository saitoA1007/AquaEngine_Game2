#include "pch.h"
#include "Player.h"
#include "FPSCounter.h"
#include <algorithm>
#include <cmath>
using namespace GameEngine;

Player::Player(const PlayerViewAssets& viewAssets, const IceAreaAssets& iceAreaAssets, InputCommand* inputCommand, const Field* field, const PlayerStats* stats)
    : inputCommand_(inputCommand), field_(field), stats_(stats), playerView_(viewAssets), iceAreaView_(iceAreaAssets) {

    RegisterDebugParameters();
}

void Player::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("Player");
    debugParam_->Register("radius", radius_);
    debugParam_->Register("initialHp", initialHp_);
    debugParam_->Register("invincibleTime", invincibleTime_);
    debugParam_->Register("blinkInterval", blinkInterval_);

    // 氷霧とチャージ。サブグループ名はここで管理する
    const std::string kMistGroup = "IceMist";
    debugParam_->Register("baseIceAreaRadius", baseIceAreaRadius_, 0, kMistGroup);
    debugParam_->Register("maxIceAreaRadius", maxIceAreaRadius_, 1, kMistGroup);
    debugParam_->Register("tapThreshold", tapThreshold_, 2, kMistGroup);
    debugParam_->Register("maxChargeTime", maxChargeTime_, 3, kMistGroup);
    debugParam_->Register("mistCooldownTime", mistCooldownTime_, 4, kMistGroup);
    debugParam_->Register("tapPower", tapPower_, 5, kMistGroup);
    debugParam_->Register("maxChargePower", maxChargePower_, 6, kMistGroup);
    debugParam_->Apply();

    // 保存済みの初期HPと氷エリアの元の半径を反映してから開始する
    hp_ = initialHp_;
    currentIceAreaRadius_ = baseIceAreaRadius_;
}

void Player::SetInputEnabled(bool isEnabled) {
    if (isInputEnabled_ == isEnabled) {
        return;
    }
    isInputEnabled_ = isEnabled;

    // 無効にするときはチャージを取り消す。氷霧は出さず、氷エリアも元の大きさへ戻す
    if (!isEnabled) {
        isCharging_ = false;
        chargeTimer_ = 0.0f;
        UpdateIceAreaRadius();
        return;
    }

    // 戻すときは、決定に使ったAを一度離すまでチャージを始めない（押したままの離しで氷霧が出るのを防ぐ）
    waitReleaseBeforeCharge_ = true;
}

void Player::Initialize() {
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

    // 氷エリアの結晶の回転と、発動時の光の時間を進める
    iceAreaView_.Update(FpsCounter::GetGameDeltaTime());

    // 無敵時間を進める
    invincibleTimer_ = (std::max)(invincibleTimer_ - FpsCounter::GetGameDeltaTime(), 0.0f);

    // 加減速なしで、入力方向へ即座に移動する。氷霧のクールタイム中も移動は止めない
    Vector3 direction = GetMoveDirection();
    moveDirection_ = direction;
    position_ +=direction * (stats_->moveSpeed * FpsCounter::GetGameDeltaTime());

    // フィールドの外へ出ないよう押し戻す
    position_ = field_->ClampToField(position_, radius_);

    // 球が地面に接するよう、半径に合わせて高さを決める
    position_.y = radius_;

    // 移動後の位置を中心に、チャージと氷霧を扱う
    UpdateIceMist(FpsCounter::GetGameDeltaTime());

    // 氷霧の瞬間（上のUpdateIceMistで始まる）を含め、見た目の状態を進める
    UpdatePlayerView(FpsCounter::GetGameDeltaTime());
}

void Player::UpdatePlayerView(float deltaTime) {
    // チャージの進み具合（0〜1）と、連鎖中か（氷塊が1つでも飛んでいるか）を見た目へ渡す
    float chargeRatio = isCharging_ ? (std::min)(chargeTimer_ / (std::max)(maxChargeTime_, 0.01f), 1.0f) : 0.0f;
    playerView_.Update(deltaTime, chargeRatio, flyingShardCount_ > 0);
}

void Player::UpdateIceMist(float deltaTime) {
    // 入力が無効の間は、チャージも氷霧も受け付けない
    if (!isInputEnabled_) {
        UpdateIceAreaRadius();
        return;
    }

    // 入力を戻した直後は、Aを一度離すまで受け付けない
    if (waitReleaseBeforeCharge_) {
        if (inputCommand_->IsCommandActive("IceMistCharge")) {
            UpdateIceAreaRadius();
            return;
        }
        waitReleaseBeforeCharge_ = false;
    }

    // クールタイム中は氷霧の入力（チャージ）を受け付けない
    if (cooldownTimer_ > 0.0f) {
        cooldownTimer_ = (std::max)(cooldownTimer_ - deltaTime, 0.0f);
        UpdateIceAreaRadius();
        return;
    }

    // 押している間はチャージする。上限に達したらその状態を保つ
    if (inputCommand_->IsCommandActive("IceMistCharge")) {
        isCharging_ = true;
        chargeTimer_ = (std::min)(chargeTimer_ + deltaTime, maxChargeTime_);
    }

    // 離した瞬間に氷霧を出す。チャージ中でなければ（クールタイム中に押した等）何もしない
    if (isCharging_ && inputCommand_->IsCommandActive("IceMistRelease")) {
        FireIceMist();
    }

    UpdateIceAreaRadius();
}

void Player::UpdateIceAreaRadius() {
    // チャージ中は経過時間に応じて元の半径からフルチャージの半径へ広がる。離したら元に戻る
    float baseRadius = baseIceAreaRadius_;
    float maxRadius = maxIceAreaRadius_;
    if (!isCharging_) {
        currentIceAreaRadius_ = baseRadius;
        return;
    }
    float ratio = (std::min)(chargeTimer_ / (std::max)(maxChargeTime_, 0.01f), 1.0f);
    currentIceAreaRadius_ = baseRadius + (maxRadius - baseRadius) * ratio;
}

void Player::FireIceMist() {
    IceMistInfo info;
    info.id = ++mistCounter_;
    info.center = position_;
    info.moveDirection = moveDirection_;

    if (chargeTimer_ < tapThreshold_) {
        // 単押し。チャージした範囲にはならず、威力は弱い
        info.radius = baseIceAreaRadius_;
        info.power = tapPower_;
    } else {
        // 長押し。チャージ中の氷エリアの半径がそのまま氷霧の範囲になり、威力は割合で上がる
        float ratio = (std::min)(chargeTimer_ / (std::max)(maxChargeTime_, 0.01f), 1.0f);
        info.radius = currentIceAreaRadius_;
        info.power = tapPower_ + (maxChargePower_ - tapPower_) * ratio;
    }

    // チャージの状態を戻し、共通のクールタイムを始めてから通知する
    isCharging_ = false;
    chargeTimer_ = 0.0f;
    cooldownTimer_ = mistCooldownTime_;

    // 氷霧の範囲の縁から光の輪を広げる
    iceAreaView_.PlayBurst(position_, info.radius);

    // 自機の核と枠を一瞬大きくする
    playerView_.PlayPulse();

    if (iceMistCallback_) {
        iceMistCallback_(info);
    }
}

bool Player::TakeDamage(int32_t damage) {
    // 無敵時間中、またはダメージがないときは何もしない
    if (IsInvincible() || damage <= 0) {
        return false;
    }

    hp_ = (std::max)(hp_ - damage, 0);
    invincibleTimer_ = invincibleTime_;
    return true;
}

void Player::ApplyPushback(const Vector3& offset) {
    // 押し戻したあともフィールド内に収める
    position_.x += offset.x;
    position_.z += offset.z;
    position_ = field_->ClampToField(position_, radius_);
    position_.y = radius_;
}

void Player::Draw() {
    // 氷エリアはプレイヤーを中心に常に描く
    iceAreaView_.Draw(renderQueue_, position_, currentIceAreaRadius_);

    // 無敵時間中は、点滅の間隔ごとに自機の描画を飛ばす（経過時間の奇数番目の間隔だけ描かない）
    if (IsInvincible()) {
        float elapsed = invincibleTime_ - invincibleTimer_;
        int32_t blinkIndex = static_cast<int32_t>(elapsed / (std::max)(blinkInterval_, 0.01f));
        if (blinkIndex % 2 == 1) {
            return;
        }
    }
    // 核と枠は描く直前の位置で行列を作るため、Updateの後の押し戻しも反映される
    playerView_.Draw(renderQueue_, position_);
}

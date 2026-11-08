#include "pch.h"
#include "PlayerUpgrade.h"
#include <cstdio>
using namespace GameEngine;

PlayerUpgrade::PlayerUpgrade() {
    RegisterDebugParameters();
}

void PlayerUpgrade::RegisterDebugParameters() {
    debugParam_ = std::make_unique<DebugParameter>("PlayerUpgrade");
    debugParam_->Register("rangeStep", rangeStep_, 0);
    debugParam_->Register("powerStep", powerStep_, 1);
    debugParam_->Register("moveSpeedStep", moveSpeedStep_, 2);
    debugParam_->Apply();
}

void PlayerUpgrade::UpdateParameters() {
    debugParam_->ApplyIfDirty();
}

const char* PlayerUpgrade::GetName(UpgradeType type) {
    switch (type) {
    case UpgradeType::kRange:
        return "範囲";
    case UpgradeType::kPower:
        return "威力";
    default:
        return "移動速度";
    }
}

std::string PlayerUpgrade::GetDescription(UpgradeType type) const {
    char buffer[64];
    switch (type) {
    case UpgradeType::kRange:
        std::snprintf(buffer, sizeof(buffer), "氷霧の範囲 +%.0f%%", rangeStep_ * 100.0f);
        break;
    case UpgradeType::kPower:
        std::snprintf(buffer, sizeof(buffer), "氷霧の威力 +%.0f%%", powerStep_ * 100.0f);
        break;
    default:
        std::snprintf(buffer, sizeof(buffer), "移動速度 +%.1f", moveSpeedStep_);
        break;
    }
    return buffer;
}

void PlayerUpgrade::Select(UpgradeType type) {
    ++counts_[static_cast<size_t>(type)];
}

void PlayerUpgrade::ApplyToStats(PlayerStats& stats, float baseMoveSpeed) const {
    stats.rangeScale = 1.0f + rangeStep_ * static_cast<float>(GetCount(UpgradeType::kRange));
    stats.powerScale = 1.0f + powerStep_ * static_cast<float>(GetCount(UpgradeType::kPower));
    stats.moveSpeed = baseMoveSpeed + moveSpeedStep_ * static_cast<float>(GetCount(UpgradeType::kMoveSpeed));
}

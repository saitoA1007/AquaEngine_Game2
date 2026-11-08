#include "pch.h"
#include "PhaseDirector.h"
#include <algorithm>
using namespace GameEngine;

PhaseDirector::PhaseDirector(const std::vector<EnemyDefinition>& definitions) {
    // フェーズ表を読み込む（不正なら例外）
    table_ = PhaseTableIO::Load(PhaseTableIO::kPhaseTablePath, definitions);

#ifdef USE_IMGUI
    editor_ = std::make_unique<PhaseTableEditor>(PhaseTableIO::kPhaseTablePath, definitions);

    // 保存されたら、今のフェーズにすぐ反映する
    editor_->SetSavedCallback([this](const PhaseTable& saved) {
        table_ = saved;
        if (ruleChangedCallback_) {
            ruleChangedCallback_();
        }
        });
    editor_->SetJumpCallback([this](int32_t phase) { JumpToPhase(phase); });
#endif
}

void PhaseDirector::DebugUpdate() {
#ifdef USE_IMGUI
    editor_->Draw(level_, exp_, GetPhase(), pendingLevelUps_);
#endif
}

void PhaseDirector::AddExp(int32_t amount) {
    // ボスのフェーズ中と、0以下の加算は無視する
    if (amount <= 0 || IsBossPhase()) {
        return;
    }

    int32_t maxLevel = static_cast<int32_t>(table_.phases.size());
    exp_ += amount;

    // 未処理の分も数えて、必要量を超えるたびにレベルアップを増やす
    while (level_ + pendingLevelUps_ < maxLevel) {
        const PhaseRow& row = table_.phases[static_cast<size_t>(level_ + pendingLevelUps_ - 1)];
        if (exp_ < row.expToNext) {
            break;
        }
        exp_ -= row.expToNext;
        ++pendingLevelUps_;
    }

    // 上限に達したら、それを超える分は数えない
    if (level_ + pendingLevelUps_ >= maxLevel) {
        exp_ = 0;
    }
}

void PhaseDirector::ConfirmLevelUp() {
    if (pendingLevelUps_ <= 0) {
        return;
    }

    --pendingLevelUps_;
    ++level_;
    NotifyBossIfNeeded();
}

void PhaseDirector::JumpToPhase(int32_t phase) {
    int32_t maxLevel = static_cast<int32_t>(table_.phases.size());
    level_ = std::clamp(phase, 1, maxLevel);
    exp_ = 0;
    pendingLevelUps_ = 0;

    if (ruleChangedCallback_) {
        ruleChangedCallback_();
    }
    NotifyBossIfNeeded();
}

float PhaseDirector::GetExpRatio() const {
    const PhaseRow& row = GetCurrentRow();
    if (row.type == PhaseType::kBoss || row.expToNext <= 0) {
        return 1.0f;
    }
    return std::clamp(static_cast<float>(exp_) / static_cast<float>(row.expToNext), 0.0f, 1.0f);
}

void PhaseDirector::NotifyBossIfNeeded() {
    if (IsBossPhase() && bossStartCallback_) {
        bossStartCallback_();
    }
}

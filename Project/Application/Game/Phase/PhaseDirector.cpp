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
    editor_->Draw(exp_, GetPhase());
#endif
}

void PhaseDirector::AddExp(int32_t amount) {
    // ボスのフェーズ中と、0以下の加算は無視する
    if (amount <= 0 || IsBossPhase()) {
        return;
    }

    int32_t maxPhase = static_cast<int32_t>(table_.phases.size());
    int32_t startPhase = phase_;
    exp_ += amount;

    // 必要量を超えるたびにフェーズを1つ進める。1回で複数フェーズ分が入っても続けて進める
    while (phase_ < maxPhase) {
        const PhaseRow& row = table_.phases[static_cast<size_t>(phase_ - 1)];
        if (exp_ < row.expToNext) {
            break;
        }
        exp_ -= row.expToNext;
        ++phase_;
    }

    // 上限に達したら、それを超える分は数えない
    if (phase_ >= maxPhase) {
        exp_ = 0;
    }

    // フェーズが進んでいなければ通知しない
    if (phase_ == startPhase) {
        return;
    }

    // 進んだ後のフェーズの出現ルールを渡し、ボスならボス開始を通知する
    if (ruleChangedCallback_) {
        ruleChangedCallback_();
    }
    NotifyBossIfNeeded();
}

void PhaseDirector::JumpToPhase(int32_t phase) {
    int32_t maxPhase = static_cast<int32_t>(table_.phases.size());
    phase_ = std::clamp(phase, 1, maxPhase);
    exp_ = 0;

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

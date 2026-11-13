#include "pch.h"
#include "PhaseTableEditor.h"
#ifdef USE_IMGUI
#include "ImguiManager.h"
#include <algorithm>
#include <exception>

PhaseTableEditor::PhaseTableEditor(const std::string& filePath, const std::vector<EnemyDefinition>& definitions)
    : filePath_(filePath), definitions_(definitions) {
    Reload();
}

void PhaseTableEditor::Reload() {
    try {
        editing_ = PhaseTableIO::Load(filePath_, definitions_);
        message_ = "loaded";
    } catch (const std::exception& e) {
        message_ = std::string("load failed: ") + e.what();
    }
}

void PhaseTableEditor::Save() {
    std::string error;
    if (!PhaseTableIO::Validate(editing_, definitions_, error)) {
        message_ = "not saved: " + error;
        return;
    }
    PhaseTableIO::Save(filePath_, editing_);
    message_ = "saved (applied to the current phase)";

    if (savedCallback_) {
        savedCallback_(editing_);
    }
}

void PhaseTableEditor::Draw(int32_t exp, int32_t phase) {
    ImGui::Begin("PhaseTable");

    // 現在の状態
    ImGui::Text("phase: %d  exp: %d", phase, exp);
    ImGui::Separator();

    if (ImGui::Button("Save")) {
        Save();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload")) {
        Reload();
    }
    ImGui::TextUnformatted(message_.c_str());
    ImGui::Separator();

    // 表が読み込めていないときは編集できない
    if (editing_.phases.empty()) {
        ImGui::End();
        return;
    }

    // 編集するフェーズを選ぶ
    int32_t phaseCount = static_cast<int32_t>(editing_.phases.size());
    ImGui::SliderInt("edit phase", &selectedPhase_, 1, phaseCount);
    selectedPhase_ = std::clamp(selectedPhase_, 1, phaseCount);
    DrawRow(editing_.phases[static_cast<size_t>(selectedPhase_ - 1)]);

    // 指定したフェーズへ飛ぶ（デバッグ用）。編集中の内容ではなく、保存済みの表の行へ移る
    ImGui::Separator();
    if (ImGui::Button("Jump to this phase")) {
        if (jumpCallback_) {
            jumpCallback_(selectedPhase_);
        }
    }

    ImGui::End();
}

void PhaseTableEditor::DrawRow(PhaseRow& row) {
    // フェーズの種類
    const PhaseType kTypes[] = { PhaseType::kNormal, PhaseType::kBonus, PhaseType::kBoss };
    if (ImGui::BeginCombo("type", PhaseTableIO::ToString(row.type))) {
        for (PhaseType type : kTypes) {
            bool isSelected = (type == row.type);
            if (ImGui::Selectable(PhaseTableIO::ToString(type), isSelected)) {
                row.type = type;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::DragInt("expToNext", &row.expToNext, 1.0f, 0, 100000);
    ImGui::DragFloat("spawnInterval", &row.spawnInterval, 0.01f, 0.05f, 60.0f);
    ImGui::DragInt("spawnPerInterval", &row.spawnPerInterval, 1.0f, 0, 1000);
    ImGui::DragInt("maxEnemies", &row.maxEnemies, 1.0f, 0, 10000);

    // 重みは敵一覧の全種類を並べる。書いていない種類は0として表示する
    ImGui::Separator();
    ImGui::TextUnformatted("weights (0 = does not spawn)");
    for (const EnemyDefinition& def : definitions_) {
        float& weight = row.weights[def.id];
        ImGui::DragFloat(def.id.c_str(), &weight, 0.05f, 0.0f, 1000.0f);
    }
}
#endif

#include "pch.h"
#include "EnemyListEditor.h"
#ifdef USE_IMGUI
#include "ImguiManager.h"
#include <cstdio>
#include <exception>

namespace {
    /// <summary>
    /// std::stringをImGuiの文字入力で編集する。
    /// </summary>
    bool InputString(const char* label, std::string& value) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
        if (ImGui::InputText(label, buffer, sizeof(buffer))) {
            value = buffer;
            return true;
        }
        return false;
    }
}

EnemyListEditor::EnemyListEditor(const std::string& filePath) : filePath_(filePath) {
    Reload();
}

void EnemyListEditor::Reload() {
    try {
        editing_ = EnemyDefinitionIO::Load(filePath_);
        message_ = "loaded";
    } catch (const std::exception& e) {
        message_ = std::string("load failed: ") + e.what();
    }
}

void EnemyListEditor::Save() {
    std::string error;
    if (!EnemyDefinitionIO::Validate(editing_, error)) {
        message_ = "not saved: " + error;
        return;
    }
    EnemyDefinitionIO::Save(filePath_, editing_);
    message_ = "saved (applies on next load)";
}

void EnemyListEditor::Draw() {
    ImGui::Begin("EnemyList");

    if (ImGui::Button("Save")) {
        Save();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload")) {
        Reload();
    }
    ImGui::SameLine();
    if (ImGui::Button("Add row")) {
        EnemyDefinition added;
        added.id = "new" + std::to_string(editing_.size());
        added.displayName = "new enemy";
        editing_.push_back(added);
    }
    ImGui::TextUnformatted(message_.c_str());
    ImGui::Separator();

    // 削除した行以降を詰めるため、削除は描画の後にまとめて行う
    int32_t removeIndex = -1;
    for (size_t i = 0; i < editing_.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        // ImGuiのフォントは日本語の字形を持たないため、表示名は英語で付ける
        std::string header = editing_[i].displayName + " (" + editing_[i].id + ")";
        if (ImGui::CollapsingHeader(header.c_str())) {
            DrawRow(editing_[i]);
            if (ImGui::Button("Delete row")) {
                removeIndex = static_cast<int32_t>(i);
            }
        }
        ImGui::PopID();
    }
    if (removeIndex >= 0) {
        editing_.erase(editing_.begin() + removeIndex);
    }

    ImGui::End();
}

void EnemyListEditor::DrawRow(EnemyDefinition& definition) {
    InputString("id", definition.id);
    // ImGuiのフォントは日本語の字形を持たないため、表示名は英語で付ける（日本語は???と表示される）
    InputString("displayName", definition.displayName);

    // 派生クラスの種類
    const std::vector<std::string>& kinds = EnemyDefinitionIO::GetKnownKinds();
    if (ImGui::BeginCombo("kind", definition.kind.c_str())) {
        for (const std::string& kind : kinds) {
            bool isSelected = (kind == definition.kind);
            if (ImGui::Selectable(kind.c_str(), isSelected)) {
                definition.kind = kind;

                // 種類が必須とする追加の値が足りなければ0で補う
                for (const std::string& name : EnemyDefinitionIO::GetRequiredExtraNames(kind)) {
                    definition.extras.emplace(name, 0.0f);
                }
            }
        }
        ImGui::EndCombo();
    }

    InputString("modelName", definition.modelName);
    ImGui::DragFloat("visualScale", &definition.visualScale, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("radius", &definition.radius, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("moveSpeed", &definition.moveSpeed, 0.05f, 0.0f, 50.0f);
    ImGui::DragFloat("iceThreshold", &definition.iceThreshold, 0.05f, 0.0f, 100.0f);
    ImGui::DragInt("contactDamage", &definition.contactDamage, 1.0f, 0, 10);
    ImGui::DragInt("exp", &definition.exp, 1.0f, 0, 1000);
    ImGui::DragInt("iceShardCount", &definition.iceShardCount, 1.0f, 0, 100);
    ImGui::DragInt("breakCount", &definition.breakCount, 1.0f, 1, 100);
    ImGui::DragInt("poolSize", &definition.poolSize, 1.0f, 0, 10000);
    ImGui::ColorEdit4("color", &definition.color.x);

    // 追加の値は、既にある名前の数値だけ編集できる
    for (std::pair<const std::string, float>& extra : definition.extras) {
        ImGui::DragFloat(extra.first.c_str(), &extra.second, 0.05f);
    }
}
#endif

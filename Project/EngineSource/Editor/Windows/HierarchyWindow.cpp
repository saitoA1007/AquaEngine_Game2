#include "pch.h"
#include "HierarchyWindow.h"
#include "GameParamEditor.h"
#include "ImGuiManager.h"
#include <format>

using namespace GameEngine;

HierarchyWindow::HierarchyWindow(GameParamEditor* gameParamEditor) {
    gameParamEditor_ = gameParamEditor;
}

void HierarchyWindow::Draw() {
    if (!isActive) return;

    if (!ImGui::Begin("Hierarchy", &isActive)) {
        ImGui::End();
        return;
    }

    const std::string& activeSceneName = gameParamEditor_->GetActiveScene();

    // シーンフィルタ表示
    if (activeSceneName.empty() || activeSceneName == "None") {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "All Groups");
        ImGui::Separator();
    } else {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Active Scene: %s", activeSceneName.c_str());
        ImGui::Separator();
    }

    const std::string& selectedRootGroupName = gameParamEditor_->GetRootGroupName();

    // 各グループをリスト表示
    for (auto& [rootGroupName, group] : gameParamEditor_->GetAllGroups()) {

        // 指定したシーンにあった項目を表示する
        if (!activeSceneName.empty() && activeSceneName != "None" &&
            !group.sceneName.empty() &&
            group.sceneName != activeSceneName) {
            continue;
        }

        bool isSelected = (selectedRootGroupName == rootGroupName);

        // 項目数を表示
        std::string label = rootGroupName;
        if (ImGui::Selectable(label.c_str(), isSelected)) {
            gameParamEditor_->SetRootGroupName(rootGroupName);
        }

        // 右クリックメニュー
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Save")) {
                gameParamEditor_->SaveFile(rootGroupName);
                std::string message = std::format("{}.json saved.", rootGroupName);
                MessageBoxA(nullptr, message.c_str(), "GameParamEditor", 0);
            }
            if (ImGui::MenuItem("Load")) {
                gameParamEditor_->LoadFile(rootGroupName);
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}
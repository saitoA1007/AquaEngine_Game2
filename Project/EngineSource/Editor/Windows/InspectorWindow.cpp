#include "pch.h"
#include "InspectorWindow.h"

using namespace GameEngine;

namespace {
    // ツールバー用のアイコンボタン
    bool IconButton(const char* id, ImTextureID tex, const char* tooltip) {
        const bool pressed = ImGui::ImageButton(id, tex, ImVec2(16, 16));
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tooltip); }
        return pressed;
    }
}

InspectorWindow::InspectorWindow(GameParamEditor* gameParamEditor, TextureManager* textureManager) {
    gameParamEditor_ = gameParamEditor;
    textureManager_ = textureManager;

    uint32_t saveIconGH = textureManager_->GetHandleByName("saveIcon.png");
    uint32_t loadIconGH = textureManager_->GetHandleByName("loadIcon.png");

    saveIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(saveIconGH);
    loadIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(loadIconGH);
}

void InspectorWindow::Draw() {
    if (!isActive) return;

    if (!ImGui::Begin("Inspector", &isActive)) {
        ImGui::End();
        return;
    }

    const std::string& selectedPath = gameParamEditor_->GetRootGroupName();

    // グループが選択されていない場合の表示
    if (selectedPath.empty()) {
        ImGui::TextDisabled("No group selected");
        ImGui::End();
        return;
    }

    // パスからルートグループ名を取得
    const std::string rootGroupName = selectedPath.substr(0, selectedPath.find('/'));

    // ルートグループが存在するかチェック
    auto& allGroups = gameParamEditor_->GetAllGroups();
    auto itRoot = allGroups.find(rootGroupName);
    if (itRoot == allGroups.end()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Group not found: %s", rootGroupName.c_str());
        gameParamEditor_->SetRootGroupName("");
        ImGui::End();
        return;
    }

    // 選択したパスをヘッダーに表示
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Group: %s", selectedPath.c_str());
    ImGui::Separator();

    // 保存ボタン
    if (IconButton("##save", static_cast<ImTextureID>(saveIconTextureHandle_.ptr), "Save")) {
        gameParamEditor_->SaveFile(rootGroupName);
        std::string message = std::format("{}.json saved.", rootGroupName);
        MessageBoxA(nullptr, message.c_str(), "GameParamEditor", 0);
    }

    ImGui::SameLine();

    // 読み込みボタン
    if (IconButton("##load", static_cast<ImTextureID>(loadIconTextureHandle_.ptr), "Load")) {
        gameParamEditor_->LoadFile(rootGroupName);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ルートグループ全体を再帰的に描画
    DrawGroup(itRoot->second);

    ImGui::End();
}

void InspectorWindow::DrawGroup(GameParamEditor::Group& group) {

    // このグループのアイテムを描画
    DrawItems(group);

    // サブグループを再帰的に
    for (auto& [childName, childGroup] : group.children) {
        ImGui::PushID(childName.c_str());

        if (ImGui::TreeNode(childName.c_str())) {
            // 再帰
            DrawGroup(childGroup);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

void InspectorWindow::DrawItems(GameParamEditor::Group& group) {
    if (group.items.empty()) { return; }

    // 優先順位でソート
    std::vector<std::pair<std::string, GameParamEditor::Item*>> sortedItems;
    for (auto& [itemName, item] : group.items) {
        sortedItems.push_back({ itemName, &item });
    }
    std::sort(sortedItems.begin(), sortedItems.end(),
        [](const auto& a, const auto& b) {
            if (a.second->priority != b.second->priority) {
                return a.second->priority < b.second->priority;
            }
            return a.first < b.first;
        }
    );

    // ソート済みの順序で描画
    for (auto& [itemName, itemPtr] : sortedItems) {
        ImGui::PushID(itemName.c_str());
        std::visit(DebugParameterVisitor{ itemName, itemPtr->isDirty, textureManager_ }, itemPtr->value);
        ImGui::Separator();
        ImGui::PopID();
    }
}
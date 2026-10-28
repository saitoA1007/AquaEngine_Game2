#include "pch.h"
#include"ConsoleWindow.h"
#include"ImGuiManager.h"
#include"LogManager.h"

using namespace GameEngine;

void ConsoleWindow::Draw() {
    ImGui::Begin("Console", &isActive);

    // ログのデータを取得取得する
    auto& logManager = LogManager::GetInstance();
    const std::vector<LogData>& logs = logManager.GetLogs();
    const std::unordered_set<std::string>& groups = logManager.GetGroups();

    if (ImGui::Button("Clear")) {
        logManager.ClearLogs();
    }
    ImGui::SameLine();
    // フィルター機能を展開する
    if (ImGui::Button("Filters")) {
        ImGui::OpenPopup("FilterPopup");
    }

    // 新しいグループがあればフィルタに追加
    for (const auto& groupName : groups) {
        if (groupFilters_.find(groupName) == groupFilters_.end()) {
            groupFilters_[groupName] = true;
        }
    }

    // どのグループをフィルターするかを管理
    if (ImGui::BeginPopup("FilterPopup")) {
        ImGui::Text("[SelectGroups]");

        if (!groups.empty()) {
            // グループをソートする
            std::vector<std::string> sortedGroups(groups.begin(), groups.end());
            std::sort(sortedGroups.begin(), sortedGroups.end());

            // グループをフィルターするかを指定する
            for (const auto& groupName : sortedGroups) {
                ImGui::Checkbox(groupName.c_str(), &groupFilters_[groupName]);
            }
        } else {
            ImGui::Text("NoneGroups");
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // ログ表示エリア
    ImGui::BeginChild("LogArea", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& log : logs) {
        // グループフィルタチェック
        auto filterIt = groupFilters_.find(log.groupName);
        if (filterIt == groupFilters_.end() || !filterIt->second) {
            continue;
        }

        // ログを表示
        ImGui::Text("[%s] : %s", log.groupName.c_str(), log.message.c_str());
    }

    // スクロールさせる
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}
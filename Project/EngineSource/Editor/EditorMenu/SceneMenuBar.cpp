#include "pch.h"
#include"SceneMenuBar.h"
#include"SceneRegistry.h"
#include"ImGuiManager.h"

using namespace GameEngine;

SceneMenuBar::SceneMenuBar(SceneChangeRequest* request) {
	sceneChangeRequest_ = request;
}

void SceneMenuBar::Run() {

    // 現在のシーン名を取得する
    const std::string& currentSceneName = sceneChangeRequest_->GetCurrentSceneName();

    // 登録されているシーン名のリストを取得
    const auto& sceneNames = sceneChangeRequest_->GetSceneNames();

    // メインメニュー
    if (ImGui::BeginMainMenuBar()) {

        if (ImGui::BeginMenu("File")) {

            if (ImGui::BeginMenu("SceneControll")) {
                // 現在のシーン表示
                ImGui::Text("CurrentScene: %s", currentSceneName.c_str());
                ImGui::Separator();

                // 現在のシーンのインデックスを取得
                int currentIndex = 0;
                for (size_t i = 0; i < sceneNames.size(); ++i) {
                    if (sceneNames[i] == currentSceneName) {
                        currentIndex = static_cast<int>(i);
                        break;
                    }
                }

                // コンボボックス用のアイテムリストを作成
                if (ImGui::BeginCombo("SelectScene", currentSceneName.c_str())) {
                    for (size_t i = 0; i < sceneNames.size(); ++i) {
                        bool isSelected = (currentIndex == static_cast<int>(i));
                        if (ImGui::Selectable(sceneNames[i].c_str(), isSelected)) {
                            if (sceneChangeRequest_) {
                                sceneChangeRequest_->RequestChange(sceneNames[i]);
                            }
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
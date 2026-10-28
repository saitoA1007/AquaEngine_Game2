#include "pch.h"
#include "EditorMenuBar.h"
#include "ImGuiManager.h"

using namespace GameEngine;

void EditorMenuBar::Run(EditorWindowManager* windowManager) {

	// メインメニュー
	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("Edit")) {
			ImGui::Text("NoneMaterialEditor");
			ImGui::EndMenu();
		}

		// ウィンドウを表示するかを管理するメニュー
		if (ImGui::BeginMenu("Window")) {
			for (auto& window : windowManager->GetWindows()) {
				ImGui::MenuItem(window->GetName().c_str(), nullptr, &window->isActive);
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}
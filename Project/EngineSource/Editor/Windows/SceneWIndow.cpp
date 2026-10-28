#include "pch.h"
#include "SceneWIndow.h"
#include "ImGuiManager.h"
#include "EditorMenu/AddObjectBar.h"
using namespace GameEngine;

SceneWindow::SceneWindow(RenderPassController* renderPassController, AddObjectBar* addObjectBar) {
	renderPassController_ = renderPassController;
	addObjectBar_ = addObjectBar;
}

void SceneWindow::Draw() {
    ImGui::Begin("GameScene", &isActive);
	// 描画した結果を移す
	ImVec2 sceneWindowSize = ImGui::GetContentRegionAvail();
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = renderPassController_->GetSrvHandle(renderPassController_->GetPresentPass());

	// 実際に使うサイズ
	ImVec2 imageSize = sceneWindowSize;

	// ウィンドウの比率
	float windowAspect = sceneWindowSize.x / sceneWindowSize.y;

	if (windowAspect > kTargetAspect) {
		// ウィンドウが横長すぎる場合、高さに合わせる
		imageSize.x = sceneWindowSize.y * kTargetAspect;
		imageSize.y = sceneWindowSize.y;
	} else {
		// ウィンドウが縦長すぎる場合、幅に合わせる
		imageSize.x = sceneWindowSize.x;
		imageSize.y = sceneWindowSize.x / kTargetAspect;
	}

	// 上を基準にする
	float offsetX = (sceneWindowSize.x - imageSize.x) * 0.5f;
	ImVec2 cursorPos = ImGui::GetCursorScreenPos();
	ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + offsetX, cursorPos.y));

	ImGui::Image((ImTextureID)srvHandle.ptr, imageSize);

	// Assetウィンドウからドラッグされたファイルをこの画像上にドロップした時の処理
	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_PATH")) {
			std::string droppedPath(static_cast<const char*>(payload->Data));
			addObjectBar_->AddObjectFromPath(droppedPath);
		}
		ImGui::EndDragDropTarget();
	}

	// 選択したオブジェクトのギズモを表示
	addObjectBar_->ApplyGuizmo();

    ImGui::End();
}
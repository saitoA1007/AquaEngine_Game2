#include "pch.h"
#include "PixWindow.h"
#include "Debug/PixCapture.h"

using namespace GameEngine;

void PixWindow::Draw() {
	ImGui::Begin("PIX Capture", &isActive);

	auto& pix = PixCapture::GetInstance();

	if (!pix.IsAvailable()) {
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "PIX capture is unavailable");
		ImGui::TextWrapped("%s", pix.GetStatusMessage().c_str());
		ImGui::Separator();
		ImGui::TextWrapped(
			"PIX for Windows をインストールし、USE_PIX を定義したビルドで実行してください。");
		ImGui::End();
		return;
	}

	ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "PIX capture ready");
	ImGui::Separator();

	ImGui::SliderInt("Frames", &captureFrameCount_, 1, 10);

	bool openAfter = pix.GetOpenAfterCapture();
	if (ImGui::Checkbox("Open in PIX after capture", &openAfter)) {
		pix.SetOpenAfterCapture(openAfter);
	}

	// 画面左上に出るPIXのオーバーレイ表示
	bool hud = pix.IsHudVisible();
	if (ImGui::Checkbox("Show PIX HUD overlay", &hud)) {
		pix.SetHudVisible(hud);
	}

	ImGui::Separator();

	if (pix.IsCapturing()) {
		ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.4f, 1.0f), "Capturing...");
	} else {
		if (ImGui::Button("Capture", ImVec2(140.0f, 32.0f))) {
			pix.RequestCapture(static_cast<uint32_t>(captureFrameCount_));
		}
		ImGui::SameLine();
		ImGui::TextDisabled("( F11 )");
	}

	ImGui::Separator();
	ImGui::TextWrapped("Status : %s", pix.GetStatusMessage().c_str());

	ImGui::TextWrapped("Output : %s", pix.GetOutputDirectoryUtf8().c_str());
	if (ImGui::Button("Open folder")) {
		pix.OpenOutputDirectory();
	}

	if (!pix.GetLastCapturePath().empty()) {
		ImGui::TextWrapped("Last : %s", pix.GetLastCapturePath().c_str());
		if (ImGui::Button("Copy path")) {
			ImGui::SetClipboardText(pix.GetLastCapturePath().c_str());
		}
	}

	ImGui::End();
}

#include "pch.h"
#include "EditorToolBar.h"
#include "ImGuiManager.h"

using namespace GameEngine;

EditorToolBar::EditorToolBar(GameEngine::TextureManager* textureManager) {

	// デバックで使用する画像を登録する
	textureManager->RegisterTexture("EngineSource/Resources/Textures/debugPlay.png");
	textureManager->RegisterTexture("EngineSource/Resources/Textures/debugPause.png");
	textureManager->RegisterTexture("EngineSource/Resources/Textures/debugStop.png");

	// 画像のsrvHandleを取得する
	playImagesrvHandle_ = textureManager->GetTextureSrvHandlesGPU(textureManager->GetHandleByName("debugPlay.png"));
	pauseImagesrvHandle_ = textureManager->GetTextureSrvHandlesGPU(textureManager->GetHandleByName("debugPause.png"));
	stopImagesrvHandle_ = textureManager->GetTextureSrvHandlesGPU(textureManager->GetHandleByName("debugStop.png"));
}

EditorToolBar::~EditorToolBar() {}

void EditorToolBar::Initialize() {

}

void EditorToolBar::Run() {

	// ショートカットキーでも更新処理を管理
	UpdateShortcutsKey();

	// メインメニュー
	if (ImGui::BeginMainMenuBar()) {
		ImVec2 imageSize = { 12.0f,12.0f };
		// playMode_ の状態に応じて表示するボタンを切り替える
		if (playMode_ == ScenePlayMode::Stop) {
			// 停止中の場合、再生する
			if (ImGui::ImageButton("PlayButton",static_cast<ImTextureID>(playImagesrvHandle_.ptr), imageSize)) {
				playMode_ = ScenePlayMode::Play;
			}
		} else if (playMode_ == ScenePlayMode::Play) {
			
			// 再生中の場合、リセットする
			if (ImGui::ImageButton("ResetButton", static_cast<ImTextureID>(stopImagesrvHandle_.ptr), imageSize)) {
				playMode_ = ScenePlayMode::Stop;
				isPause_ = false;
			}
		}

		ImGui::SameLine();

		// 一時停止する
		if (ImGui::ImageButton("PauseButton", static_cast<ImTextureID>(pauseImagesrvHandle_.ptr), imageSize)) {
			if (playMode_ == ScenePlayMode::Play) {
				if (isPause_) {
					isPause_ = false;
				} else {
					isPause_ = true;
				}
			}
		}

		ImGui::EndMainMenuBar();
	}

}

bool EditorToolBar::GetIsActiveUpdate() const {
	switch (playMode_)
	{
	case ScenePlayMode::Play:
		return true;
		break;

	case ScenePlayMode::Stop:
		return false;
		break;
	}
	return true;
}

void EditorToolBar::UpdateShortcutsKey() {
	ImGuiIO& io = ImGui::GetIO();

	// Ctrl + Pで再生
	if (io.KeyCtrl && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_P, false)) {

		if (playMode_ == ScenePlayMode::Play) {
			playMode_ = ScenePlayMode::Stop;
		}else if (playMode_ == ScenePlayMode::Stop) {
			playMode_ = ScenePlayMode::Play;
		}
	}

	// Ctrl + Alt + Pで停止する
	if (io.KeyCtrl && io.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_P, false)) {
		playMode_ = ScenePlayMode::Stop;
		isPause_ = false;
	}

	// Ctrl + Shift + Pで一時停止
	if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_P, false)) {
		if (playMode_ == ScenePlayMode::Play) {
			if (isPause_) {
				isPause_ = false;
			} else {
				isPause_ = true;
			}
		}	
	}
}
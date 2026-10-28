#pragma once
#include "IEditorWindow.h"
#include "ImGuiManager.h"

namespace GameEngine {

	/// <summary>
	/// PIXのGPUキャプチャを操作するエディタウィンドウ
	/// </summary>
	class PixWindow : public IEditorWindow {
	public:
		void Draw() override;
		std::string GetName() const override { return "PIXCapture"; }

	private:
		int captureFrameCount_ = 1;
	};
}

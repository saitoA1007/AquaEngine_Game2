#pragma once
#include"IEditorWindow.h"
#include"RenderPass/RenderPassController.h"

namespace GameEngine {

	class AddObjectBar;

	class SceneWindow : public IEditorWindow {
	public:

		SceneWindow(RenderPassController* renderPassController, AddObjectBar* addObjectBar);

		void Draw() override;
		std::string GetName() const override { return "SceneView"; };

	private:
		GameEngine::RenderPassController* renderPassController_ = nullptr;
		AddObjectBar* addObjectBar_ = nullptr;
		// 固定したいアスペクト比 (16:9)
		const float kTargetAspect = 1280.0f / 720.0f;
	};
}

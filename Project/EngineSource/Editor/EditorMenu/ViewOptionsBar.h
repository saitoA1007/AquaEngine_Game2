#pragma once
#include "WorldTransform.h"

namespace GameEngine {

	class Input;
	class RenderQueue;
	class Model;
	class DebugCamera;
	class DebugRenderer;

	class ViewOptionsBar {
	public:
		ViewOptionsBar(Input* input, RenderQueue* renderQueue, DebugRenderer* debugRenderer, Model* gridModel);
		~ViewOptionsBar();

		void Run();

		DebugCamera* GetDebugCamera() { return debugCamera_.get(); }

	private:
		// デバック画面の描画
		bool isDebugView_ = true;

		// デバック描画を表示
		bool isDebugDraw_ = true;

		// 描画コマンド
		RenderQueue* renderQueue_ = nullptr;
		// デバック描画機能
		DebugRenderer* debugRenderer_ = nullptr;

		// グリッドを描画するためのモデル
		Model* gridModel_;
		WorldTransform gridWorldTransform_;

		// デバックカメラ
		std::unique_ptr<DebugCamera> debugCamera_;
	};
}

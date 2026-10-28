#pragma once
#include "TextureManager.h"

namespace GameEngine {

	enum class ScenePlayMode {
		Play,  // 再生
		Stop,  // 停止
	};

	class EditorToolBar {
	public:

		EditorToolBar(GameEngine::TextureManager* textureManager);
		~EditorToolBar();

		void Initialize();

		void Run();

		// 状態取得
		ScenePlayMode GetPlayMode() const { return playMode_; }

		// 更新しているかを取得
		bool GetIsActiveUpdate() const;

		// 一時停止を取得する
		bool GetIsPauce() const { return isPause_; }

	private:
		D3D12_GPU_DESCRIPTOR_HANDLE playImagesrvHandle_;
		D3D12_GPU_DESCRIPTOR_HANDLE pauseImagesrvHandle_;
		D3D12_GPU_DESCRIPTOR_HANDLE stopImagesrvHandle_;

		// 実行状態を管理する
		ScenePlayMode playMode_ = ScenePlayMode::Stop;

		bool isPause_ = false;

	private:

		/// <summary>
		/// ショートカットキーでも行えるようにする
		/// </summary>
		void UpdateShortcutsKey();
	};
}

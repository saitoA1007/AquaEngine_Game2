#pragma once
#include <vector>
#include <filesystem>
#include "IEditorWindow.h"
#include <d3d12.h>

namespace GameEngine {

	// 前方宣言
	class TextureManager;

	class AssetWindow : public IEditorWindow {
	public:
		AssetWindow(TextureManager* textureManager);

		void Draw() override;
		std::string GetName() const override { return "Asset"; };

	private:
		TextureManager* textureManager_ = nullptr;

		const std::filesystem::path resourcesPath = "Resources";
		// 現在開いているフォルダ
		std::filesystem::path selectedPath = "Resources";

		// 画像ハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE folderIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE meshIconTextureHandle_{};
		D3D12_GPU_DESCRIPTOR_HANDLE jsonIconTextureHandle_{};

		// エラーハンドル
		D3D12_GPU_DESCRIPTOR_HANDLE errorTextureHandle_{};

	private:

		// フォルダ階層を再帰的にツリー表示する
		void RenderDirectoryTree(const std::filesystem::path& path);

		// 選択されたフォルダの中身をグリッド表示する
		void RenderContentArea();
	};
}

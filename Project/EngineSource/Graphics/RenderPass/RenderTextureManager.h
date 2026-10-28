#pragma once
#include <unordered_map>
#include "RtvManager.h"
#include "SrvManager.h"
#include "DsvManager.h"
#include "RenderTexture.h"

namespace GameEngine {

	class RenderTextureManager {
	public:
		RenderTextureManager() = default;
		~RenderTextureManager() = default;

		void Initialize(RtvManager* rtvMasnager,DsvManager* dsvmanager, ID3D12Device5* device);

		/// <summary>
		/// レンダーテクスチャを作成して登録する
		/// </summary>
		void Create(
			const std::string& name,
			uint32_t width,uint32_t height,
			RenderTextureMode  mode = RenderTextureMode::RtvAndDsv,
			DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			Vector4 clearColor = { 0.2f,0.2f,0.2f,1.0f }
		);

		/// <summary>
		/// 登録済みのレンダーテクスチャを取得する
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		[[nodiscard]]
		RenderTexture* GetRenderTexture(const std::string& name) const;

		/// <summary>
		/// 登録済みのレンダーテクスチャを解放する
		/// </summary>
		/// <param name="name"></param>
		void Release(const std::string& name);

		/// <summary>
		/// 全てのレンダーテクスチャを解放する
		/// </summary>
		void ReleaseAll();

	private:
		RenderTextureManager(const RenderTextureManager&) = delete;
		RenderTextureManager& operator=(const RenderTextureManager&) = delete;

		ID3D12Device5* device_ = nullptr;
		std::unordered_map<std::string, std::unique_ptr<RenderTexture>> renderTextures_;
	};
}
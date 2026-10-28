#pragma once
#include <d3d12.h>
#include <cstdint>

namespace GameEngine {

	class DXViewportState final {
	public:

		DXViewportState() = default;
		~DXViewportState() = default;

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="width">横幅</param>
		/// <param name="height">縦幅</param>
		void Initialize(uint32_t width, uint32_t height);

		//void Apply(ID3D12GraphicsCommandList* commandList) const;

		const D3D12_VIEWPORT& GetViewport() const { return viewport_; }

		const D3D12_RECT& GetScissorRect() const { return scissorRect_; }

	private:

		// ビューポート
		D3D12_VIEWPORT viewport_{};
		// シザー矩形
		D3D12_RECT scissorRect_{};
	};
}


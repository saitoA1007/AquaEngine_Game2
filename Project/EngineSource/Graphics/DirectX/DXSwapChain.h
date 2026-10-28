#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include<cstdint>

namespace GameEngine {

	class DXSwapChain final {
	public:

		DXSwapChain() = default;
		~DXSwapChain() = default;

		void Initialize(HWND hwnd, uint32_t width, uint32_t height, IDXGIFactory7* dxgiFactory, ID3D12CommandQueue* commandQueue);

		DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc() const { return swapChainDesc; }

		void Present();

		IDXGISwapChain4* GetSwapChain() const { return swapChain_.Get(); }
		uint32_t GetBackBufferIndex() const { return static_cast<uint32_t>(swapChain_->GetCurrentBackBufferIndex()); }

	private:

		Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	};
}


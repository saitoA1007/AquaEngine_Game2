#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include<cstdint>

namespace GameEngine {

	class DXRenderTarget final {
	public:

		DXRenderTarget() = default;
		~DXRenderTarget() = default;

		void Initialize(ID3D12Device* device,IDXGISwapChain4* swapChain);

		D3D12_CPU_DESCRIPTOR_HANDLE GetSwapChainRTVHandle(const uint32_t& index) const { return rtvHandles_[index]; }
		ID3D12Resource* GetSwapChainResource(const uint32_t& index) const { return swapChainResources_[index].Get(); }

		ID3D12DescriptorHeap* GetRTVHeap() const { return rtvHeap_.Get(); }
		ID3D12DescriptorHeap* GetDSVHeap() const { return dsvHeap_.Get(); }

		uint32_t GetRTVDescriptorSize() const { return descriptorSizeRTV_; }
		uint32_t GetDSVDescriptorSize() const { return descriptorSizeDSV_; }
	private:

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;

		uint32_t descriptorSizeRTV_ = 0;
		uint32_t descriptorSizeDSV_ = 0;

		static const uint32_t kSwapChainBufferCount = 2;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[kSwapChainBufferCount] = {};
		Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[kSwapChainBufferCount];
	};
}


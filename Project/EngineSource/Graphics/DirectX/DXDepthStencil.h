#pragma once
#include <d3d12.h>
#include <wrl.h>
#include"Externals/DirectXTex/d3dx12.h"
#include"SrvManager.h"

namespace GameEngine {

	class DXDepthStencil final {
	public:

		DXDepthStencil() = default;
		~DXDepthStencil() = default;

		void Initialize(ID3D12Device* device, ID3D12DescriptorHeap* dsvHeap, uint32_t width, uint32_t height, SrvManager* srvManager);

		CD3DX12_GPU_DESCRIPTOR_HANDLE GetSRVHandle() const { return depthSRVHandle_; }

		ID3D12Resource* GetResource() const { return depthStencilResource_.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;

		// 深度値を取得するためのSRVハンドル
		CD3DX12_GPU_DESCRIPTOR_HANDLE depthSRVHandle_;
	};
}


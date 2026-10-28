#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

// リソースバッファを作成
Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device5* device, size_t sizeInBytes,
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD,
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ,
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

/// <summary>
/// リソースの状態を切り替え
/// </summary>
/// <param name="commandList">コマンド</param>
/// <param name="resource">リソース</param>
/// <param name="before">前の状態</param>
/// <param name="after">後の状態</param>
void TransitionResource(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource,
	D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

/// <summary>
/// リソースの作成
/// </summary>
Microsoft::WRL::ComPtr<ID3D12Resource> CreateResource(ID3D12Device* device, const D3D12_RESOURCE_DESC& resourceDesc,
	D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState,
	const D3D12_CLEAR_VALUE* clearValue = nullptr,
	D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE);

/// <summary>
/// テクスチャ用設定
/// </summary>
D3D12_RESOURCE_DESC CreateTexture2dDesc(uint32_t width, uint32_t height, DXGI_FORMAT format,
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, uint16_t mipLevels = 1);
#include "pch.h"
#include "CreateBufferResource.h"
#include <cassert>
#include <cstdlib>
#include <Windows.h>

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device5* device, size_t sizeInBytes,
	D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags) {
	// ヒープの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = heapType;

	// リソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;// リソースのサイズ。
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = flags;

	// リソースを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&resource));
	if (FAILED(hr)) {
		// Release版でも確実に検知できるように
		OutputDebugStringA("[CreateBufferResource] CreateCommittedResource failed\n");
		assert(false && "CreateCommittedResourceに失敗しました");
		std::abort();
	}
	return resource;
}

void TransitionResource(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource,
	D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
	if (before == after) { return; }
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateResource(ID3D12Device* device, const D3D12_RESOURCE_DESC& resourceDesc, 
	D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState,
	const D3D12_CLEAR_VALUE* clearValue, D3D12_HEAP_FLAGS heapFlags) {
	// ヒープ設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = heapType;

	// リソース作成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		heapFlags,
		&resourceDesc,
		initialState,
		clearValue,
		IID_PPV_ARGS(&resource));
	if (FAILED(hr)) {
		// Release版でも確実に検知できるように
		OutputDebugStringA("[CreateResource] CreateCommittedResource failed\n");
		assert(false && "リソースの作成に失敗しました");
		std::abort();
	}
	return resource;
}

D3D12_RESOURCE_DESC CreateTexture2dDesc(uint32_t width, uint32_t height,
	DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flags, uint16_t mipLevels) {
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = mipLevels;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;
	return desc;
}
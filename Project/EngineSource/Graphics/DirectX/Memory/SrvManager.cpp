#include "pch.h"
#include "SrvManager.h"
#include "DescriptorHandle.h"
#include "DescriptorHeap.h"
#include <cassert>

using namespace GameEngine;

void SrvManager::Initialize(ID3D12Device* device) {
	device_ = device;
	// SRV用のヒープを作成する
	srvHeap_ = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSrvIndex_, true);
	// DescriptorSizeを取得しておく
	descriptorSizeSRV_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 領域を分割する
	uint32_t startIndex = 0;
	ranges_[SrvHeapType::Texture] = { startIndex, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0,    {} };

	startIndex = static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount);
	ranges_[SrvHeapType::System] = { startIndex, startIndex + static_cast<uint32_t>(SrvHeapTypeCount::SystemMaxCount), startIndex, {} };

	startIndex = startIndex + static_cast<uint32_t>(SrvHeapTypeCount::SystemMaxCount);
	ranges_[SrvHeapType::AccessData] = { startIndex, startIndex + static_cast<uint32_t>(SrvHeapTypeCount::AccessMaxCount), startIndex, {} };

	startIndex = startIndex + static_cast<uint32_t>(SrvHeapTypeCount::AccessMaxCount);
	ranges_[SrvHeapType::Buffer] = { startIndex, startIndex + static_cast<uint32_t>(SrvHeapTypeCount::BufferMaxCount), startIndex, {} };

	startIndex = startIndex + static_cast<uint32_t>(SrvHeapTypeCount::BufferMaxCount);
	ranges_[SrvHeapType::Other] = { startIndex, startIndex + static_cast<uint32_t>(SrvHeapTypeCount::OtherMaxCount), startIndex, {} };
}

uint32_t SrvManager::AllocateSrvIndex(SrvHeapType srvHeapType) {
	HeapRange& range = ranges_[srvHeapType];

	uint32_t index = 0;

	// 利用可能であれば使用する
	if (!range.freeList.empty()) {
		index = range.freeList.front();
		range.freeList.pop();
	} else if (range.current < range.end) {
		// 新規割り当て
		index = range.current++;
	} else {
		// 枯渇エラー
		assert(0 && "Descriptor Heap Range Run Out");
		return 0;
	}

	// インデックスのタイプを設定
	indexTypeMap_[index] = srvHeapType;

	return index;
}

void SrvManager::ReleaseIndex(uint32_t index) {
	// インデックスがどのタイプか検索
	auto it = indexTypeMap_.find(index);
	if (it != indexTypeMap_.end()) {
		SrvHeapType type = it->second;
		// 該当する領域のフリーリストに戻す
		ranges_[type].freeList.push(index);
		// マップからは削除
		indexTypeMap_.erase(it);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUHandle(uint32_t index) const {
	return GetCPUDescriptorHandle(srvHeap_.Get(), descriptorSizeSRV_, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUHandle(uint32_t index) const {
	return GetGPUDescriptorHandle(srvHeap_.Get(), descriptorSizeSRV_, index);
}

uint32_t SrvManager::GetStartSrvIndex(SrvHeapType type) {
	return ranges_[type].start;
}
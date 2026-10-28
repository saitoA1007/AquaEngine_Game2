#include "pch.h"
#include "RtvManager.h"
#include "LogManager.h"
#include "DescriptorHeap.h"
#include "DescriptorHandle.h"
using namespace GameEngine;

void RtvManager::Initialize(ID3D12Device* device) {
    LogManager::GetInstance().Log("RtvManager start Initialize");

    device_ = device;

    // RTV用のヒープ
    rtvHeap_ = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, maxRtvCount_, false);
    // サイズを取得
    descriptorSizeRTV_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    LogManager::GetInstance().Log("RtvManager end Initialize\n");
}

uint32_t RtvManager::CreateView(ID3D12Resource* resource, DXGI_FORMAT format) {
    assert(resource && "resource が null です");

    uint32_t index = AllocateIndex();

    // 外部所有のリソースなので resources_[index] には格納しない
    CreateRtvDescriptor(index, resource, format);

    return index;
}

void RtvManager::ReleaseIndex(uint32_t index) {
    assert(index < maxRtvCount_ && "RTV index out of range");
    // 解放されたインデックスを再利用リストに追加
    freeIndices_.push_back(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE RtvManager::GetCPUHandle(uint32_t index) const {
    assert(index < maxRtvCount_ && "RTV index out of range");
    return GetCPUDescriptorHandle(rtvHeap_.Get(), descriptorSizeRTV_, index);
}

uint32_t RtvManager::AllocateIndex() {
    if (!freeIndices_.empty()) {
        uint32_t index = freeIndices_.front();
        freeIndices_.pop_front();
        return index;
    }
    assert(nextRtvIndex_ < maxRtvCount_ && "RTV heap is full");
    return nextRtvIndex_++;
}

void RtvManager::CreateRtvDescriptor(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format) {
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandle(rtvHeap_.Get(), descriptorSizeRTV_, index);
    device_->CreateRenderTargetView(resource, &rtvDesc, handle);
}

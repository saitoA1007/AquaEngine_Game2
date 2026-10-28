#include "pch.h"
#include "DsvManager.h"
#include <cassert>
#include "DepthStencilTexture.h"
#include "DescriptorHeap.h"
#include "DescriptorHandle.h"
using namespace GameEngine;

void DsvManager::Initialize(ID3D12Device* device) {
    device_ = device;

    // DSV用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
    dsvHeap_ = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, maxDsvCount_, false);
    descriptorSizeDSV_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

uint32_t DsvManager::CreateView(ID3D12Resource* resource, DXGI_FORMAT dsvFormat) {
    assert(resource && "resource が null です");

    uint32_t index = AllocateIndex();

    // 外部所有リソースなので resources_[index] には格納しない
    CreateDsvDescriptor(index, resource, dsvFormat);

    return index;
}

void DsvManager::ReleaseIndex(uint32_t index) {
    assert(index < maxDsvCount_);
    freeIndices_.push_back(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DsvManager::GetCPUHandle(uint32_t index) const {
    return GetCPUDescriptorHandle(dsvHeap_.Get(), descriptorSizeDSV_, index);
}

uint32_t DsvManager::AllocateIndex() {
    if (!freeIndices_.empty()) {
        uint32_t index = freeIndices_.front();
        freeIndices_.pop_front();
        return index;
    }
    assert(nextDsvIndex_ < maxDsvCount_ && "DSV heap is full");
    return nextDsvIndex_++;
}

void DsvManager::CreateDsvDescriptor(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT dsvFormat) {
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = dsvFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandle(dsvHeap_.Get(), descriptorSizeDSV_, index);
    device_->CreateDepthStencilView(resource, &dsvDesc, handle);
}

DXGI_FORMAT DsvManager::ToTypelessFormat(DXGI_FORMAT dsvFormat) {
    switch (dsvFormat) {
    case DXGI_FORMAT_D32_FLOAT:         return DXGI_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24G8_TYPELESS;
    case DXGI_FORMAT_D16_UNORM:         return DXGI_FORMAT_R16_TYPELESS;
    default:
        assert(false && "未対応の DSV フォーマットです");
        return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT DsvManager::ToSrvFormat(DXGI_FORMAT dsvFormat) {
    switch (dsvFormat) {
    case DXGI_FORMAT_D32_FLOAT:         return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case DXGI_FORMAT_D16_UNORM:         return DXGI_FORMAT_R16_UNORM;
    default:
        assert(false && "未対応の DSV フォーマットです");
        return DXGI_FORMAT_UNKNOWN;
    }
}
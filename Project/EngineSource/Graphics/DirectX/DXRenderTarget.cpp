#include "pch.h"
#include"DXRenderTarget.h"
#include"LogManager.h"
#include"DescriptorHeap.h"
#include"DescriptorHandle.h"
#include"Debug/DebugName.h"
using namespace GameEngine;

void DXRenderTarget::Initialize(ID3D12Device* device, IDXGISwapChain4* swapChain) {

    LogManager::GetInstance().Log("Start　Create TargetView");

    // RTVの設定
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_ = {};
    rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    // RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisibleはfalse
    rtvHeap_ = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
    // DSV用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
    dsvHeap_ = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

    // DescriptorSizeを取得しておく
    descriptorSizeRTV_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptorSizeDSV_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    HRESULT hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
    // うまく取得できなければ起動できない
    assert(SUCCEEDED(hr));
    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
    assert(SUCCEEDED(hr));

    // RTVを2つ作るのでディスクリプタを2つ用意する(スワップチェーンに使用する画面2つ)
    // まず1つ目を作る。
    rtvHandles_[0] = GetCPUDescriptorHandle(rtvHeap_.Get(), descriptorSizeRTV_, 0);
    device->CreateRenderTargetView(swapChainResources_[0].Get(), &rtvDesc_, rtvHandles_[0]);
    // 2つ目のディスクリプタハンドルを得る(自力で)
    rtvHandles_[1] = GetCPUDescriptorHandle(rtvHeap_.Get(), descriptorSizeRTV_, 1);
    // 2つ目を作る
    device->CreateRenderTargetView(swapChainResources_[1].Get(), &rtvDesc_, rtvHandles_[1]);

    // PIX上で識別できるように名前を付ける
    SetDebugName(rtvHeap_.Get(), "RTVHeap");
    SetDebugName(dsvHeap_.Get(), "DSVHeap");
    SetDebugName(swapChainResources_[0].Get(), "BackBuffer", 0);
    SetDebugName(swapChainResources_[1].Get(), "BackBuffer", 1);

    LogManager::GetInstance().Log("End　Create TargetView");
}
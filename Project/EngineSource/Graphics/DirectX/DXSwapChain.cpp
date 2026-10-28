#include "pch.h"
#include"DXSwapChain.h"
#include<cassert>
#include"LogManager.h"

using namespace GameEngine;

void DXSwapChain::Initialize(HWND hwnd, uint32_t width, uint32_t height, IDXGIFactory7* dxgiFactory, ID3D12CommandQueue* commandQueue) {

    LogManager::GetInstance().Log("Start　Create SwapChain");

    // スワップチェーンを生成する
    swapChainDesc.Width = width;                       // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
    swapChainDesc.Height = height;                     // 画面の高さ。ウィンドウのクライアント領域同じものにしておく
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
    swapChainDesc.SampleDesc.Count = 1;  // マルチサンプル
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // 描画をターゲットとして利用する
    swapChainDesc.BufferCount = 2;  // ダブルバッファー
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // モニタに映ったら、中身を破棄

    // コマンドキュー、ウィンドウハンドル、設定を渡して生成する
    HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.ReleaseAndGetAddressOf()));
    assert(SUCCEEDED(hr));

    LogManager::GetInstance().Log("End　Create SwapChain");
}

void DXSwapChain::Present() {
    swapChain_->Present(1, 0);
}
#include "pch.h"
#include"DXFence.h"
#include<cassert>
#include"LogManager.h"
using namespace GameEngine;


void DXFence::Initialize(ID3D12Device* device) {

    LogManager::GetInstance().Log("Start　Create Fence");

    // 初期値0でFenceを作る
    HRESULT hr = device->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    assert(SUCCEEDED(hr));

    // FenceのSignalを待つためのイベントを作成する
    fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent_ != nullptr);

    LogManager::GetInstance().Log("Start　Create Fence");
}

void DXFence::WaitForGPU(ID3D12CommandQueue* commandQueue) {
    // GPUの完了を待つ
    // Fenceの値を更新
    fenceValue_++;
    // GPUがここまでたどり着いた時に、Fenceの値を指定した値を代入するようにSignalを送る
    commandQueue->Signal(fence_.Get(), fenceValue_);

    // Fenceの値が指定したSignal値にたどり着いているかを確認する
    // GetCompletedValueの初期値はFence作成時に渡した初期値
    if (fence_->GetCompletedValue() < fenceValue_) {
        // 指定したSignalにたどりついていないので、たどり着くまで待つようにイベントを設定する
        fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
        // イベントを待つ
        WaitForSingleObject(fenceEvent_, INFINITE);
    }
}
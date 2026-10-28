#include "pch.h"
#include "ResourceGarbageCollector.h"
#include "DXFence.h"
using namespace GameEngine;

void ResourceGarbageCollector::Add(Microsoft::WRL::ComPtr<ID3D12Resource> resource) {
    releaseQueue_.push({ resource, fence_->GetNextFenceValue() });
}

void ResourceGarbageCollector::ProcessCompletedResources() {
    uint64_t completedValue = fence_->GetCompletedValue();

    // キューの先頭から、完了したフェンス値以下か確認する
    while (!releaseQueue_.empty()) {
        auto& item = releaseQueue_.front();

        if (item.fenceValue <= completedValue) {
            // 破棄する
            item.resource.Reset();
            releaseQueue_.pop();
        } else {
            // まだ実行中のリソースに当たったらそれ以降も実行中なので抜ける
            break;
        }
    }
}
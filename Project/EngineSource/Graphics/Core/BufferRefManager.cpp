#include "pch.h"
#include "BufferRefManager.h"
#include <cassert>
using namespace GameEngine;

void BufferRefManager::Initialize() {
    // アクセスデータを作成
    bufferRefs_.Create(maxCount_, SrvHeapType::AccessData);
}

uint32_t BufferRefManager::AllocateIndex() {
    if (!freeIndices_.empty()) {
        uint32_t index = freeIndices_.front();
        freeIndices_.pop_front();
        return index;
    }
    assert(nextIndex_ < maxCount_ && "MaterialRef heap is full");
    return nextIndex_++;
}

void BufferRefManager::ReleaseIndex(const uint32_t& index) {
    assert(index < maxCount_ && "MaterialRef index out of range");
    // 解放されたインデックスを再利用リストに追加
    freeIndices_.push_back(index);
}

BufferRef* BufferRefManager::GetBufferRef(const uint32_t& index) {
    assert(index < maxCount_ && "MaterialRef index out of range");

    auto* data = bufferRefs_.GetData();
    return &data[index];
}
#include "pch.h"
#include"DXViewportState.h"

using namespace GameEngine;

void DXViewportState::Initialize(uint32_t width, uint32_t height) {
    // クライアント領域のサイズと一緒にして画面全体に表示
    viewport_.Width = static_cast<float>(width);
    viewport_.Height = static_cast<float>(height);
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    // 基本的にビューポートと同じ矩形が構成されるようにする
    scissorRect_.left = 0;
    scissorRect_.right = static_cast<int>(width);
    scissorRect_.top = 0;
    scissorRect_.bottom = static_cast<int>(height);
}
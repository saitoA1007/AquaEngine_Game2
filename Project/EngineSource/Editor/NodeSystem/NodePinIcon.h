#pragma once
#include "ImGuiManager.h"
#include "MaterialGraph.h"

namespace GameEngine::NodeUI {

    enum class PinIconType {
        Circle,    // シンプルな丸
        Data,      // 丸 + 右向き三角 (データ型ピン)
        Flow       // D字型 (実行フロー/メインパス用)
    };

    // ピンアイコン描画関数
    void DrawPinIcon(PinIconType type, bool connected, ImU32 color, float size = 14.0f);

    // ピンの型ごとの色
    ImU32 PinTypeColor(PinType type);
}
#include "pch.h"
#include "NodePinIcon.h"

namespace GameEngine::NodeUI {

    void DrawPinIcon(PinIconType type, bool connected, ImU32 color, float size) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        switch (type) {
        case PinIconType::Circle: {
            ImVec2 c(p.x + size * 0.5f, p.y + size * 0.5f);
            float r = size * 0.32f;
            if (connected) dl->AddCircleFilled(c, r, color, 16);
            else dl->AddCircle(c, r, color, 16, 2.0f);
            break;
        }
        case PinIconType::Data: {
            ImVec2 c(p.x + size * 0.42f, p.y + size * 0.5f);
            float r = size * 0.22f;
            if (connected) dl->AddCircleFilled(c, r, color, 24);
            else dl->AddCircle(c, r, color, 24, 2.0f);

            float tx = c.x + r + 3.0f;
            dl->AddTriangleFilled(ImVec2(tx, c.y - 3.5f), ImVec2(tx, c.y + 3.5f), ImVec2(tx + 5.0f, c.y), color);
            break;
        }
        case PinIconType::Flow: {
            const float l = p.x + size * 0.20f, mid = p.x + size * 0.55f, r = p.x + size * 0.90f;
            const float t = p.y + size * 0.20f, b = p.y + size * 0.80f, cy = p.y + size * 0.50f;
            ImVec2 pts[5] = { {l, t}, {mid, t}, {r, cy}, {mid, b}, {l, b} };
            if (connected) dl->AddConvexPolyFilled(pts, 5, color);
            dl->AddPolyline(pts, 5, color, ImDrawFlags_Closed, 2.0f);
            break;
        }
        }

        ImGui::Dummy(ImVec2(size, size));
    }

    ImU32 PinTypeColor(PinType type) {
        switch (type) {
        case PinType::kFloat:     return IM_COL32(147, 226, 74, 255);   // 緑
        case PinType::kFloat2:    return IM_COL32(0, 230, 180, 255);    // ターコイズ
        case PinType::kFloat3:    return IM_COL32(255, 200, 40, 255);   // 黄
        case PinType::kFloat4:    return IM_COL32(240, 90, 200, 255);   // ピンク
        case PinType::kTexture2D: return IM_COL32(51, 150, 215, 255);   // 青
        case PinType::kBool:      return IM_COL32(220, 48, 48, 255);    // 赤
        }
        return IM_COL32(200, 200, 200, 255);
    }
}


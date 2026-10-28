#include "pch.h"
#include "NodeBuilder.h"

namespace GameEngine::NodeUI {

    NodeBuilder::NodeBuilder(const NodeStyle& style) : style_(style) {}

    void NodeBuilder::Begin(ned::NodeId id) {
        nodeId_ = id;
        contentTop_ = -1.0f;
        headerBottom_ = 0.0f;
        ned::BeginNode(nodeId_);
        ImGui::PushID(nodeId_.AsPointer());
        originX_ = ImGui::GetCursorScreenPos().x;   // 右寄せの基準
    }

    void NodeBuilder::Header(const std::string& title, ImU32 tint) {
        BeginHeader(title, tint);
        EndHeader();
    }

    void NodeBuilder::BeginHeader(const std::string& title, ImU32 tint) {
        headerTint_ = tint;
        ImGui::BeginGroup();
        // ノード幅を確定
        ImGui::Dummy(ImVec2(style_.width, 2.0f));   
        ImGui::TextUnformatted(title.c_str());
    }

    void NodeBuilder::EndHeader() {
        ImGui::Dummy(ImVec2(0, 2.0f));
        ImGui::EndGroup();
        headerBottom_ = ImGui::GetItemRectMax().y;
        ImGui::Spacing();
    }

    void NodeBuilder::InputPin(ned::PinId id, const std::string& name, PinIconType icon, ImU32 color, bool connected) {
        ned::BeginPin(id, ned::PinKind::Input);
        ned::PinPivotAlignment(ImVec2(0.0f, 0.5f));
        ned::PinPivotSize(ImVec2(0, 0));
        DrawPinIcon(icon, connected, color, style_.iconSize);
        if (!name.empty()) {
            ImGui::SameLine();
            // アイコンと縦中央を揃える
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (style_.iconSize - ImGui::GetTextLineHeight()) * 0.5f);
            ImGui::TextUnformatted(name.c_str());
        }
        ned::EndPin();
    }

    void NodeBuilder::OutputPin(ned::PinId id, const std::string& name, PinIconType icon, ImU32 color, bool connected) {
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float textW = name.empty() ? 0.0f : ImGui::CalcTextSize(name.c_str()).x + spacing;
        const float itemW = textW + style_.iconSize;

        // ノード左端からの現在位置。右端に揃えるための空きを Dummy で作る
        const float curX = ImGui::GetCursorScreenPos().x - originX_;
        const float gap = style_.width - curX - itemW;
        if (gap > 0.0f) {
            ImGui::Dummy(ImVec2(gap, style_.iconSize));
            ImGui::SameLine(0.0f, 0.0f);
        }

        ned::BeginPin(id, ned::PinKind::Output);
        ned::PinPivotAlignment(ImVec2(1.0f, 0.5f));
        ned::PinPivotSize(ImVec2(0, 0));
        if (!name.empty()) {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (style_.iconSize - ImGui::GetTextLineHeight()) * 0.5f);
            ImGui::TextUnformatted(name.c_str());
            ImGui::SameLine();
        }
        DrawPinIcon(icon, connected, color, style_.iconSize);
        ned::EndPin();
    }

    void NodeBuilder::PinPair(ned::PinId inId, const std::string& inName, PinIconType inIcon, ImU32 inColor, bool inConnected,
        ned::PinId outId, const std::string& outName, PinIconType outIcon, ImU32 outColor, bool outConnected) {
        InputPin(inId, inName, inIcon, inColor, inConnected);
        ImGui::SameLine();
        OutputPin(outId, outName, outIcon, outColor, outConnected);
    }

    void NodeBuilder::BeginContent() {
        ImGui::Spacing();
        contentTop_ = ImGui::GetCursorScreenPos().y;
        ImGui::Dummy(ImVec2(0, 4.0f));
        contentStartY_ = ImGui::GetCursorScreenPos().y;
    }

    void NodeBuilder::EndContent() {
        if (ImGui::GetCursorScreenPos().y == contentStartY_) { contentTop_ = -1.0f; }
    }

    void NodeBuilder::Preview(ImTextureID tex, bool dim, float aspect) {
        ImGui::Spacing();
       //const ImVec2 size(style_.width, style_.width / aspect);
       //const ImVec2 p = ImGui::GetCursorScreenPos();
       //const ImVec2 q(p.x + size.x, p.y + size.y);
       //ImGui::Dummy(size);
       //
       //ImDrawList* dl = ImGui::GetWindowDrawList();
       //if (tex != ImTextureID{}) {
       //    const ImU32 tint = dim ? IM_COL32(255, 255, 255, 60) : IM_COL32_WHITE;
       //    dl->AddImageRounded(tex, p, q, ImVec2(0, 0), ImVec2(1, 1), tint, 4.0f);
       //} else {
       //    dl->AddRectFilled(p, q, IM_COL32(20, 20, 22, 255), 4.0f);
       //}
       // dl->AddRect(p, q, IM_COL32(255, 255, 255, 40), 4.0f);
        DrawImagePreview(tex, ImVec2(style_.width, style_.width / aspect), dim);
    }

    void NodeBuilder::End() {
        ImGui::PopID();
        ned::EndNode();

        const ImVec2 pos = ned::GetNodePosition(nodeId_);
        const ImVec2 size = ned::GetNodeSize(nodeId_);
        const auto& st = ned::GetStyle();
        ImDrawList* bg = ned::GetNodeBackgroundDrawList(nodeId_);

        const float half = st.NodeBorderWidth * 0.5f;
        const ImVec2 hMin(pos.x + half, pos.y + half);
        const ImVec2 hMax(pos.x + size.x - half, headerBottom_ + 4.0f);

        if (style_.headerTexture != ImTextureID{}) {
            // 画像をタイル状に貼る
            const ImVec2 uv((hMax.x - hMin.x) / (4.0f * style_.headerTexSize.x),
                (hMax.y - hMin.y) / (4.0f * style_.headerTexSize.y));
            bg->AddImageRounded(style_.headerTexture, hMin, hMax, ImVec2(0, 0), uv, headerTint_,
                st.NodeRounding - 1.0f, ImDrawFlags_RoundCornersTop);
        } else {
            bg->AddRectFilled(hMin, hMax, headerTint_, st.NodeRounding - 1.0f, ImDrawFlags_RoundCornersTop);
        }
        bg->AddLine(ImVec2(hMin.x, hMax.y), ImVec2(hMax.x, hMax.y), IM_COL32(255, 255, 255, 40), 1.0f);

        if (contentTop_ >= 0.0f) {
            bg->AddLine(ImVec2(pos.x + 8.0f, contentTop_ + 2.0f),
                ImVec2(pos.x + size.x - 8.0f, contentTop_ + 2.0f),
                IM_COL32(255, 255, 255, 30), 1.0f);
        }
    }

    void ApplyEditorStyle() {
        auto& style = ned::GetStyle();
        style.NodePadding = ImVec4(8, 4, 8, 8);
        style.NodeRounding = 12.0f;
        style.NodeBorderWidth = 1.0f;
        style.HoveredNodeBorderWidth = 3.0f;
        style.SelectedNodeBorderWidth = 3.0f;
        style.LinkStrength = 120.0f;
        style.FlowMarkerDistance = 30.0f;
        style.FlowSpeed = 150.0f;
        style.FlowDuration = 2.0f;

        style.Colors[ned::StyleColor_Flow] = ImColor(255, 200, 80, 255);
        style.Colors[ned::StyleColor_FlowMarker] = ImColor(255, 230, 150, 255);
        style.Colors[ned::StyleColor_Bg] = ImColor(26, 26, 28, 255);
        style.Colors[ned::StyleColor_Grid] = ImColor(255, 255, 255, 18);
        style.Colors[ned::StyleColor_NodeBg] = ImColor(32, 32, 32, 220);
        style.Colors[ned::StyleColor_NodeBorder] = ImColor(255, 255, 255, 96);
        style.Colors[ned::StyleColor_HovNodeBorder] = ImColor(255, 255, 255, 160);
        style.Colors[ned::StyleColor_SelNodeBorder] = ImColor(255, 170, 40, 255);
        style.Colors[ned::StyleColor_PinRect] = ImColor(0, 0, 0, 0);
        style.Colors[ned::StyleColor_PinRectBorder] = ImColor(0, 0, 0, 0);
    }

    void DrawImagePreview(ImTextureID tex, ImVec2 size, bool dim) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImVec2 q(p.x + size.x, p.y + size.y);
        ImGui::Dummy(size);   // 固定サイズなので、ノードが伸縮しない

        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (tex != ImTextureID{}) {
            const ImU32 tint = dim ? IM_COL32(255, 255, 255, 60) : IM_COL32_WHITE;
            dl->AddImageRounded(tex, p, q, ImVec2(0, 0), ImVec2(1, 1), tint, 4.0f);
        } else {
            dl->AddRectFilled(p, q, IM_COL32(20, 20, 22, 255), 4.0f);
            const char* label = "No Image";
            const ImVec2 ts = ImGui::CalcTextSize(label);
            dl->AddText(ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f),
                IM_COL32(120, 120, 125, 255), label);
        }
        dl->AddRect(p, q, IM_COL32(255, 255, 255, 40), 4.0f);
    }

}
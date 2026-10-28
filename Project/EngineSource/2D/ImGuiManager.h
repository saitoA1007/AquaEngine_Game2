#pragma once
#include <dxgi1_6.h>

#include"WindowsApp.h"
#include"SrvManager.h"

#ifdef USE_IMGUI
#include "Externals/imgui/imgui.h"
#include "Externals/imgui/imgui_impl_dx12.h"
#include "Externals/imgui/imgui_impl_win32.h"
#include "Externals/imgui/ImGuizmo.h"

#include "Externals/imgui/imgui_node_editor.h"
#endif

namespace GameEngine {

	class ImGuiManager final {
	public:
		ImGuiManager() = default;
		~ImGuiManager() = default;

		/// <summary>
		/// 初期処理
		/// </summary>
		/// <param name="windowsApp"></param>
		/// <param name="dxCommon"></param>
		void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, DXGI_SWAP_CHAIN_DESC1 swapChainDesc,
			WindowsApp* windowsApp,SrvManager* srvManager);

		/// <summary>
		/// 更新前処理
		/// </summary>
		void BeginFrame();

		/// <summary>
		/// 更新後処理
		/// </summary>
		void EndFrame();

		/// <summary>
		/// 描画処理
		/// </summary>
		void Draw();

		/// <summary>
		/// 終了処理
		/// </summary>
		void Finalize();

	private:
		ImGuiManager(const ImGuiManager&) = delete;
		const ImGuiManager& operator=(const ImGuiManager&) = delete;

		ID3D12GraphicsCommandList* commandList_ = nullptr;
		WindowsApp* windowsApp_ = nullptr;
		SrvManager* srvManager_ = nullptr;

	private:

		/// <summary>
		/// Imguiのスタイルを適応
		/// </summary>
		void ApplyStyle();
	};

    // 各型
    bool DrawFloat2Control(const char* l, float* v, float w = 0.0f);
    bool DrawFloat3Control(const char* l, float* v, float w = 0.0f);
    bool DrawFloat4Control(const char* l, float* v, float w = 0.0f);
    bool DrawInt2Control(const char* l, int* v, float w = 0.0f);
    bool DrawInt3Control(const char* l, int* v, float w = 0.0f);

    static const char* FindLabelEnd(const char* label)
    {
        const char* p = label;
        while (*p != '\0')
        {
            if (p[0] == '#' && p[1] == '#') {
                break;
            }
            ++p;
        }
        return p;
    }

	// 各軸の項目に色が付いた調整ウィジェット関数
    template <typename T>
    bool DrawVectorControl(const char* label, T* values, int components, float columnWidth = 100.0f, float speed = 0.01f,
        T minV = 0, T maxV = 0, const char* format = nullptr)
    {
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, int>);
        bool changed = false;
#ifdef USE_IMGUI
        if (!label) label = "##null";
        ImGui::PushID(label);

        const bool showLabel = !(label[0] == '#' && label[1] == '#');
        const int  columnCount = showLabel ? 2 : 1;

        if (ImGui::BeginTable("VecTable", columnCount, ImGuiTableFlags_SizingFixedFit))
        {
            float labelWidth = columnWidth;
            if (showLabel && labelWidth <= 0.0f)
            {
                const float textW = ImGui::CalcTextSize(label, FindLabelEnd(label)).x;
                labelWidth = textW + ImGui::GetStyle().CellPadding.x * 2.0f;
            }

            if (showLabel)
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, labelWidth);
            ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            int col = 0;
            if (showLabel)
            {
                ImGui::TableSetColumnIndex(col++);
                ImGui::AlignTextToFramePadding();
                // ##以降を隠す
                ImGui::TextUnformatted(label, FindLabelEnd(label));
            }
            ImGui::TableSetColumnIndex(col);

            const float spacing = 4.0f;
            const float availW = ImGui::GetContentRegionAvail().x;
            const float itemWidth = (availW - spacing * (components - 1)) / components;

            static const ImU32 barColors[4] = {
                IM_COL32(220, 50, 30, 255), IM_COL32(110, 190, 20, 255),
                IM_COL32(30, 120, 230, 255), IM_COL32(220, 220, 220, 255)
            };
            static const char* ids[4] = { "##X", "##Y", "##Z", "##W" };

            constexpr ImGuiDataType dt = std::is_same_v<T, float> ? ImGuiDataType_Float : ImGuiDataType_S32;
            const char* defaultFmt = std::is_same_v<T, float> ? "   %.2f" : "   %d";
            const char* fmt = format ? format : defaultFmt;
            const bool  hasRange = (minV != maxV);

            for (int i = 0; i < components; ++i)
            {
                if (i > 0) ImGui::SameLine(0, spacing);
                ImGui::SetNextItemWidth(itemWidth);

                if (ImGui::DragScalar(ids[i], dt, &values[i], speed,
                    hasRange ? &minV : nullptr,
                    hasRange ? &maxV : nullptr, fmt))
                    changed = true;

                const ImVec2 pMin = ImGui::GetItemRectMin();
                const ImVec2 pMax = ImGui::GetItemRectMax();
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImVec2(pMin.x + 2.0f, pMin.y + 2.0f),
                    ImVec2(pMin.x + 2.0f + 3.5f, pMax.y - 2.0f),
                    barColors[i], 2.0f, ImDrawFlags_RoundCornersLeft);
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
#endif
        return changed;
    }
}

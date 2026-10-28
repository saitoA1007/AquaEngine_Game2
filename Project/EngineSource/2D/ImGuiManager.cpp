#include "pch.h"
#include "ImGuiManager.h"
using namespace GameEngine;

void ImGuiManager::Initialize([[maybe_unused]]ID3D12Device* device, [[maybe_unused]] ID3D12GraphicsCommandList* commandList, [[maybe_unused]] DXGI_SWAP_CHAIN_DESC1 swapChainDesc,
	[[maybe_unused]] WindowsApp* windowsApp, [[maybe_unused]] SrvManager* srvManager) {
#ifdef USE_IMGUI
	commandList_ = commandList;
	windowsApp_ = windowsApp;
	srvManager_ = srvManager;

	// ImGuiの初期化。
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	
	// 使用するフォントを読み込む
	ImFont* baseFont = io.Fonts->AddFontFromFileTTF("EngineSource/Resources/Text/JetBrainsMono-Regular.ttf", 16.0f);
	if (baseFont == nullptr) {
		assert(false && "Imgui dont load font");
	}

	// フォントをビルド
	io.Fonts->Build();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_ = {};
	rtvDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc_.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	uint32_t index = srvManager_->AllocateSrvIndex(SrvHeapType::Other);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvManager_->GetCPUHandle(index);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvManager_->GetGPUHandle(index);

    ApplyStyle();

	ImGui_ImplWin32_Init(windowsApp_->GetHwnd());
	ImGui_ImplDX12_Init(device,
		swapChainDesc.BufferCount,
		rtvDesc_.Format,
		srvManager_->GetSRVHeap(),
		cpuHandle,
		gpuHandle);
#endif
}

void ImGuiManager::BeginFrame() {
#ifdef USE_IMGUI
	// ImGuiにフレームが始まる旨を伝える
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	//ImGui::Begin("NodeEditor");
	//ImGui::Separator();
	//ed::SetCurrentEditor(g_NodeContext);
	//ed::Begin("My Editor", ImVec2(0.0, 0.0f));
	//int uniqueId = 1;
	//// Start drawing nodes.
	//ed::BeginNode(uniqueId++);
	//ImGui::Text("Node A");
	//ed::BeginPin(uniqueId++, ed::PinKind::Input);
	//ImGui::Text("-> In");
	//ed::EndPin();
	//ImGui::SameLine();
	//ed::BeginPin(uniqueId++, ed::PinKind::Output);
	//ImGui::Text("Out ->");
	//ed::EndPin();
	//ed::EndNode();
	//ed::End();
	//ed::SetCurrentEditor(nullptr);
	//ImGui::End();
#endif
}

void ImGuiManager::EndFrame() {
#ifdef USE_IMGUI
	// ImGuiの内部コマンドを生成する
	ImGui::Render();
#endif
}

void ImGuiManager::Draw() {
#ifdef USE_IMGUI
	// 実際のcommandListのImGuiの描画コマンドを積む
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_);
#endif
}

void ImGuiManager::Finalize() {
#ifdef USE_IMGUI
	// ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}

void ImGuiManager::ApplyStyle() {
#ifdef USE_IMGUI
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // レイアウトの調整
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(6, 4);
    // 境界線の設定
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;

    /// カラーパレット定義
    ImVec4 c_black = ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
    ImVec4 c_darker = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    ImVec4 c_dark = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    // ボタン・フレームの調整
    ImVec4 c_mid = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    ImVec4 c_light = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    ImVec4 c_lighter = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    // 通常時の境界線
    ImVec4 c_border = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    // アクティブ強調カラー
    ImVec4 c_active_blue = ImVec4(0.00f, 0.15f, 0.45f, 1.00f);
    ImVec4 c_active_hover = ImVec4(0.15f, 0.45f, 0.85f, 1.00f);
    ImVec4 c_active_dark = ImVec4(0.00f, 0.10f, 0.40f, 1.00f);
    // 文字
    ImVec4 c_text = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
    ImVec4 c_text_disabled = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);

    colors[ImGuiCol_Text] = c_text;
    colors[ImGuiCol_TextDisabled] = c_text_disabled;
    // ウィンドウ背景
    colors[ImGuiCol_WindowBg] = c_darker;
    colors[ImGuiCol_ChildBg] = c_dark;
    colors[ImGuiCol_PopupBg] = c_dark;
    // 枠線
    colors[ImGuiCol_Border] = c_border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    // 通常のフレーム背景
    colors[ImGuiCol_FrameBg] = c_black;
    colors[ImGuiCol_FrameBgHovered] = c_mid;
    colors[ImGuiCol_FrameBgActive] = c_light;
    // タイトルバー
    colors[ImGuiCol_TitleBg] = c_black;
    colors[ImGuiCol_TitleBgActive] = c_active_dark;
    colors[ImGuiCol_TitleBgCollapsed] = c_black;
    // メニューバー
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);;
    // スクロールバー
    colors[ImGuiCol_ScrollbarBg] = c_black;
    colors[ImGuiCol_ScrollbarGrab] = c_mid;
    colors[ImGuiCol_ScrollbarGrabHovered] = c_light;
    colors[ImGuiCol_ScrollbarGrabActive] = c_active_blue;
    // チェックボックスのチェックマーク
    colors[ImGuiCol_CheckMark] = c_active_blue;
    // スライダーなどのつまみ
    colors[ImGuiCol_SliderGrab] = c_mid;
    colors[ImGuiCol_SliderGrabActive] = c_active_blue;
    // ボタン
    colors[ImGuiCol_Button] = c_mid;
    colors[ImGuiCol_ButtonHovered] = c_light;
    colors[ImGuiCol_ButtonActive] = c_active_blue;
    // ヘッダー
    colors[ImGuiCol_Header] = c_active_blue;
    colors[ImGuiCol_HeaderHovered] = c_active_hover;
    colors[ImGuiCol_HeaderActive] = c_active_dark;
    // セパレーター
    colors[ImGuiCol_Separator] = c_border;
    colors[ImGuiCol_SeparatorHovered] = c_active_blue;
    colors[ImGuiCol_SeparatorActive] = c_active_hover;
    // サイズ変更ハンドル
    colors[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ResizeGripHovered] = c_light;
    colors[ImGuiCol_ResizeGripActive] = c_active_blue;
    // 選択中のタブ
    colors[ImGuiCol_Tab] = c_dark;
    colors[ImGuiCol_TabHovered] = c_light;
    colors[ImGuiCol_TabActive] = ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = c_dark;
    colors[ImGuiCol_TabUnfocusedActive] = c_mid;
    // ドッキングのプレビュー
    colors[ImGuiCol_DockingPreview] = ImVec4(0.00f, 0.45f, 0.85f, 0.60f);
    colors[ImGuiCol_DockingEmptyBg] = c_black;
    // テーブルの背景
    colors[ImGuiCol_TableHeaderBg] = c_mid;
    colors[ImGuiCol_TableBorderStrong] = c_border;
    colors[ImGuiCol_TableBorderLight] = c_border;
    colors[ImGuiCol_TableRowBg] = c_darker;
    colors[ImGuiCol_TableRowBgAlt] = c_dark;
    // テキスト選択時の背景色
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.45f, 0.85f, 0.40f);
    colors[ImGuiCol_DragDropTarget] = c_active_hover;
    colors[ImGuiCol_NavHighlight] = c_active_blue;
#endif
}

namespace GameEngine {

    bool DrawFloat2Control(const char* l, float* v, float w) {
        return DrawVectorControl<float>(l, v, 2, w, 0.01f, 0.0f, 0.0f, "%.3f");
    }
    bool DrawFloat3Control(const char* l, float* v, float w) {
        return DrawVectorControl<float>(l, v, 3, w, 0.01f, 0.0f, 0.0f, "%.3f");
    }
    bool DrawFloat4Control(const char* l, float* v, float w) {
        return DrawVectorControl<float>(l, v, 4, w, 0.01f, 0.0f, 0.0f, "%.3f");
    }
    bool DrawInt2Control(const char* l, int* v, float w) { 
        return DrawVectorControl<int>(l, v, 2, w, 1.0f);
    }
    bool DrawInt3Control(const char* l, int* v, float w) { 
        return DrawVectorControl<int>(l, v, 3, w, 1.0f);
    }
}
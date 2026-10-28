#include "pch.h"
#include"WindowsApp.h"
#include"ImGuiManager.h"

#pragma comment(lib,"winmm.lib")

using namespace GameEngine;

#ifdef USE_IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

LRESULT CALLBACK WindowsApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

#ifdef USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}
#endif

	// メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		// ウィンドウが破棄された
	case WM_DESTROY:
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WindowsApp::CreateGameWindow(const std::wstring& title, int32_t kClientWidth, int32_t kClientHeight) {

	// COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	/// ウィンドウクラスの作成
	// ウィンドウプロシージャ
	wc_.lpfnWndProc = &WindowProc;
	// ウィンドウクラス名
	wc_.lpszClassName = L"CG2WindowClass";
	// インスタンスハンドル
	wc_.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// ウィンドウクラスを登録する
	RegisterClass(&wc_);

#ifdef USE_IMGUI
	// ウィンドウスタイル(サイズ変更は無効)
	const DWORD style = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME;
#else
	// ウィンドウスタイル(最大化・サイズ変更は無効)
	const DWORD style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
#endif

	int32_t posX = CW_USEDEFAULT;
	int32_t posY = CW_USEDEFAULT;
	int32_t windowWidth = 0;
	int32_t windowHeight = 0;

#ifdef USE_IMGUI
	// Debug/Development版はタスクバーを除いた作業領域いっぱいにウィンドウを表示する
	RECT workArea{};
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
	posX = workArea.left;
	posY = workArea.top;
	windowWidth = workArea.right - workArea.left;
	windowHeight = workArea.bottom - workArea.top;
	(void)kClientWidth;
	(void)kClientHeight;
#else
	// Release版は指定されたクライアント領域のサイズで表示する
	wrc_ = { 0,0,kClientWidth,kClientHeight };

	// クライアント領域を元に実際のサイズをwrcを変更してもらう
	AdjustWindowRect(&wrc_, style, false);
	windowWidth = wrc_.right - wrc_.left;
	windowHeight = wrc_.bottom - wrc_.top;
#endif

	hwnd_ = CreateWindow(
		wc_.lpszClassName,      // 利用するクラス名
		title.c_str(),          // タイトルバーの文字
		style,                  // ウィンドウスタイル
		posX,                   // 表示X座標
		posY,                   // 表示Y座標
		windowWidth,            // ウィンドウ横幅
		windowHeight,           // ウィンドウ縦幅
		nullptr,                // 親ウィンドウハンドル
		nullptr,                // メニューハンドル
		wc_.hInstance,          // インスタンスハンドル
		nullptr);               // オプション

	// 実際に作成されたクライアント領域のサイズを取得する
	RECT clientRect{};
	GetClientRect(hwnd_, &clientRect);
	clientWidth_ = clientRect.right - clientRect.left;
	clientHeight_ = clientRect.bottom - clientRect.top;

	// システムタイマーの分解能を上げる
	timeBeginPeriod(1);
}

void WindowsApp::ShowGameWindow() {
	// ウィンドウを表示して前面に出す
	ShowWindow(hwnd_, SW_SHOW);
	SetForegroundWindow(hwnd_);
}

bool WindowsApp::ProcessMessage() {

	// falseならそのまま処理、trueなら終了
	while (PeekMessage(&msg_, nullptr, 0, 0, PM_REMOVE)) {
		if (msg_.message == WM_QUIT) {
			return true;
		}
		TranslateMessage(&msg_);
		DispatchMessage(&msg_);
	}
	return false;
}

void WindowsApp::BreakGameWindow() {
	// COMの終了処理
	CoUninitialize();

	CloseWindow(hwnd_);
}
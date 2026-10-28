#include "pch.h"
#include "LoadingWIndow.h"
using namespace GameEngine;

LoadingWindow::~LoadingWindow() {
	Close();
}

void LoadingWindow::Show(const std::wstring& title, const std::wstring& message) {
	if (thread_.joinable()) { return; }

	title_ = title;
	message_ = message;

	// ウィンドウが生成されるまで待つ
	std::promise<void> created;
	std::future<void> future = created.get_future();
	thread_ = std::thread([this, p = std::move(created)]() mutable { ThreadMain(p); });
	future.wait();
}

void LoadingWindow::Close() {
	if (!thread_.joinable()) { return; }

	// ウィンドウスレッドに終了を依頼して待つ
	if (HWND hwnd = hwnd_.load()) {
		PostMessage(hwnd, WM_CLOSE, 0, 0);
	}
	thread_.join();
	hwnd_ = nullptr;
}

void LoadingWindow::SetMessage(const std::wstring& message) {
	{
		std::lock_guard<std::mutex> lock(messageMutex_);
		message_ = message;
	}
	if (HWND hwnd = hwnd_.load()) {
		InvalidateRect(hwnd, nullptr, FALSE);
	}
}

LRESULT CALLBACK LoadingWindow::WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {

	// 生成時に渡したthisを保存しておく
	if (message == WM_NCCREATE) {
		auto* cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
	}
	auto* self = reinterpret_cast<LoadingWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (message) {
	case WM_CREATE:
		SetTimer(hwnd, kAnimTimerId, kAnimIntervalMs, nullptr);
		return 0;

	case WM_TIMER:
		if (self && wparam == kAnimTimerId) {
			self->animFrame_++;
			InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;

	case WM_ERASEBKGND:
		// ちらつき防止のため背景消去はOnPaintで行う
		return 1;

	case WM_PAINT:
		if (self) {
			self->OnPaint(hwnd);
			return 0;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		KillTimer(hwnd, kAnimTimerId);
		// このスレッドのメッセージループだけを終了する
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}


void LoadingWindow::ThreadMain(std::promise<void>& created) {
	HINSTANCE hInstance = GetModuleHandle(nullptr);

	/// ウィンドウクラスの作成
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = &WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_APPSTARTING);
	wc.lpszClassName = kClassName;
	RegisterClassExW(&wc);

	// 画面中央に配置する
	int32_t x = (GetSystemMetrics(SM_CXSCREEN) - kWidth) / 2;
	int32_t y = (GetSystemMetrics(SM_CYSCREEN) - kHeight) / 2;

	HWND hwnd = CreateWindowExW(
		WS_EX_APPWINDOW,        // タスクバーに表示する
		kClassName,
		title_.c_str(),
		WS_POPUP | WS_BORDER,   // タイトルバー無し
		x, y, kWidth, kHeight,
		nullptr, nullptr, hInstance,
		this);                  // WindowProcでthisを取り出す

	hwnd_ = hwnd;
	if (hwnd) {
		ShowWindow(hwnd, SW_SHOW);
		UpdateWindow(hwnd);
	}
	created.set_value();

	if (hwnd) {
		// このスレッド専用のメッセージループ
		MSG msg{};
		while (GetMessage(&msg, nullptr, 0, 0) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	UnregisterClassW(kClassName, hInstance);
}

void LoadingWindow::OnPaint(HWND hwnd) {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	RECT rc;
	GetClientRect(hwnd, &rc);
	int32_t width = rc.right - rc.left;
	int32_t height = rc.bottom - rc.top;

	// ダブルバッファで描画する
	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP bitmap = CreateCompatibleBitmap(hdc, width, height);
	HGDIOBJ oldBitmap = SelectObject(memDC, bitmap);

	// 背景
	HBRUSH bgBrush = CreateSolidBrush(RGB(20, 20, 26));
	FillRect(memDC, &rc, bgBrush);
	DeleteObject(bgBrush);

	SetBkMode(memDC, TRANSPARENT);
	SetTextColor(memDC, RGB(240, 240, 240));

	// タイトル
	HFONT titleFont = CreateFontW(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Meiryo UI");
	HGDIOBJ oldFont = SelectObject(memDC, titleFont);
	RECT titleRect = { 0, 30, width, 80 };
	DrawTextW(memDC, title_.c_str(), -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	// メッセージとアニメーション
	std::wstring message;
	{
		std::lock_guard<std::mutex> lock(messageMutex_);
		message = message_;
	}
	uint32_t dotCount = (animFrame_ / 10) % 4;
	message.append(dotCount, L'.');

	HFONT messageFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Meiryo UI");
	SelectObject(memDC, messageFont);
	SetTextColor(memDC, RGB(180, 180, 190));
	RECT messageRect = { 0, 90, width, 120 };
	DrawTextW(memDC, message.c_str(), -1, &messageRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	// 往復するバー
	const int32_t barMargin = 40;
	const int32_t barTop = height - 40;
	const int32_t barHeight = 6;
	RECT barBgRect = { barMargin, barTop, width - barMargin, barTop + barHeight };
	HBRUSH barBgBrush = CreateSolidBrush(RGB(50, 50, 60));
	FillRect(memDC, &barBgRect, barBgBrush);
	DeleteObject(barBgBrush);

	const int32_t trackWidth = width - barMargin * 2;
	const int32_t blockWidth = trackWidth / 4;
	const int32_t range = trackWidth - blockWidth;
	const int32_t period = 60; // 往復にかかるフレーム数
	int32_t phase = static_cast<int32_t>(animFrame_ % period);
	int32_t offset = (phase < period / 2) ? (range * phase * 2 / period) : (range * (period - phase) * 2 / period);
	RECT barRect = { barMargin + offset, barTop, barMargin + offset + blockWidth, barTop + barHeight };
	HBRUSH barBrush = CreateSolidBrush(RGB(80, 170, 255));
	FillRect(memDC, &barRect, barBrush);
	DeleteObject(barBrush);

	// 画面に転送
	BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

	// 後片付け
	SelectObject(memDC, oldFont);
	DeleteObject(titleFont);
	DeleteObject(messageFont);
	SelectObject(memDC, oldBitmap);
	DeleteObject(bitmap);
	DeleteDC(memDC);

	EndPaint(hwnd, &ps);
}
#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

namespace GameEngine {

	class WindowsApp final {
	public:
		WindowsApp() = default;
		~WindowsApp() = default;

		// ウィンドウサイズ
		static const int32_t kWindowWidth = 1280;
		static const int32_t kWindowHeight = 720;

	public:

		/// <summary>
		/// ウィンドウプロシージャ
		/// </summary>
		/// <param name="hwnd">ウィンドウハンドル</param>
		/// <param name="msg"></param>
		/// <param name="wparam"></param>
		/// <param name="lparam"></param>
		/// <returns></returns>
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		/// <summary>
		/// ウィンドウの作成
		/// </summary>
		/// <param name="title">ウィンドウタイトル</param>
		/// <param name="kClientWidth">ウィンドウのクライアント領域の横幅</param>
		/// <param name="kClientHeight">ウィンドウのクライアント領域の縦幅</param>
		void CreateGameWindow(const std::wstring& title, int32_t clientWidth = kWindowWidth, int32_t clientHeight = kWindowHeight);

		/// <summary>
		/// ウィンドウを表示する
		/// </summary>
		void ShowGameWindow();

		/// <summary>
		/// メッセージ処理
		/// </summary>
		bool ProcessMessage();

		/// <summary>
		/// ウィンドウの破棄
		/// </summary>
		void BreakGameWindow();

		/// <summary>
		/// ウィンドウハンドルの取得
		/// </summary>
		/// <returns></returns>
		HWND GetHwnd() const { return hwnd_; }

		HINSTANCE GetHInstance() const { return wc_.hInstance; }

		/// <summary>
		/// 実際に作成されたクライアント領域のサイズを取得
		/// </summary>
		int32_t GetClientWidth() const { return clientWidth_; }
		int32_t GetClientHeight() const { return clientHeight_; }
	private:
		//WindowsApp() = default;
		//~WindowsApp() = default;
		WindowsApp(const WindowsApp&) = delete;
		WindowsApp& operator=(const WindowsApp&) = delete;

		// ウィンドウクラス
		WNDCLASS wc_{};
		// ウィンドウハンドル
		HWND hwnd_ = nullptr;
		RECT wrc_{};

		// 実際のクライアント領域のサイズ
		int32_t clientWidth_ = kWindowWidth;
		int32_t clientHeight_ = kWindowHeight;

		MSG msg_{};
	};
}

#pragma once
#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <future>
#include <mutex>
#include <string>
#include <thread>

namespace GameEngine {

	/// <summary>
	/// ロート中に表示するウィンドウメッセージ
	/// </summary>
	class LoadingWindow final {
	public:
		LoadingWindow() = default;
		~LoadingWindow();
	
		/// <summary>
		/// ロードウィンドウを表示する
		/// </summary>
		/// <param name="title">表示するタイトル</param>
		/// <param name="message">表示するメッセージ</param>
		void Show(const std::wstring& title, const std::wstring& message = L"Now_Loading");

		/// <summary>
		/// ロードウィンドウを閉じる
		/// </summary>
		void Close();

		/// <summary>
		/// 表示するメッセージを変更する
		/// </summary>
		/// <param name="message">表示するメッセージ</param>
		void SetMessage(const std::wstring& message);

	private:
		
		// ウィンドウサイズ
		static constexpr int32_t kWidth = 480;
		static constexpr int32_t kHeight = 180;

		static constexpr const wchar_t* kClassName = L"LoadingWindowClass";

		static constexpr UINT_PTR kAnimTimerId = 1;

		// アニメーションの更新間隔
		static constexpr UINT kAnimIntervalMs = 33;

	private:
		LoadingWindow(const LoadingWindow&) = delete;
		LoadingWindow& operator=(const LoadingWindow&) = delete;

		std::thread thread_;
		std::atomic<HWND> hwnd_ = nullptr;

		std::wstring title_;
		std::wstring message_;
		std::mutex messageMutex_;

		// アニメーション用のカウンタ
		uint32_t animFrame_ = 0;

	private:

		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

		// ウィンドウスレッドの処理
		void ThreadMain(std::promise<void>& created);

		// 描画処理
		void OnPaint(HWND hwnd);
	};
}
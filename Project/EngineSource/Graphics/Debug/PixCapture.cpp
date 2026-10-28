#include "pch.h"
#include "PixCapture.h"
#include "PixMarker.h"
#include "ConvertString.h"
#include "LogManager.h"

#include <shellapi.h>
#include <cstdio>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "shell32.lib")

using namespace GameEngine;

namespace {
	// HRESULT付きのログを出す
	void LogHResult(const char* message, HRESULT hr) {
		char buffer[256]{};
		sprintf_s(buffer, "PixCapture : %s (hr=0x%08lX)", message, static_cast<unsigned long>(hr));
		LogManager::GetInstance().Log(buffer);
	}
}

PixCapture& PixCapture::GetInstance() {
	static PixCapture instance;
	return instance;
}

bool PixCapture::Initialize() {
#if defined(USE_PIX)

	// すでにロード済みならそのまま使える
	if (GetModuleHandleW(L"WinPixGpuCapturer.dll") != nullptr) {
		isAvailable_ = true;
		statusMessage_ = "WinPixGpuCapturer.dll already loaded";
		LogManager::GetInstance().Log("PixCapture : capturer already loaded");
		return true;
	}

	// インストール済みのPIXから最新のキャプチャDLLを読み込む
	if (PIXLoadLatestWinPixGpuCapturerLibrary() == nullptr) {
		isAvailable_ = false;
		statusMessage_ = "PIX for Windows is not installed (WinPixGpuCapturer.dll not found)";
		LogHResult("PIXLoadLatestWinPixGpuCapturerLibrary failed", HRESULT_FROM_WIN32(GetLastError()));
		return false;
	}

	isAvailable_ = true;
	statusMessage_ = "Ready";

	LogManager::GetInstance().Log("PixCapture : initialized");
	return true;

#else
	isAvailable_ = false;
	statusMessage_ = "Built without USE_PIX";
	LogManager::GetInstance().Log("PixCapture : USE_PIX is not defined. GPU capture is disabled.");
	return false;
#endif
}

void PixCapture::SetHudVisible(bool visible) {
	isHudVisible_ = visible;

#if defined(USE_PIX)
	if (!isAvailable_) { return; }

	// PIX_HUD_SHOW_ON_NO_WINDOWS でオーバーレイが消える
	const PIXHUDOptions options = visible
		? PIX_HUD_SHOW_ON_TARGET_WINDOW_ONLY
		: PIX_HUD_SHOW_ON_NO_WINDOWS;

	const HRESULT hr = PIXSetHUDOptions(options);
	if (FAILED(hr)) {
		LogHResult("PIXSetHUDOptions failed", hr);
	}
#endif
}

void PixCapture::SetTargetWindow(HWND hwnd) {
	targetWindow_ = hwnd;
#if defined(USE_PIX)
	if (!isAvailable_ || hwnd == nullptr) { return; }

	const HRESULT hr = PIXSetTargetWindow(hwnd);
	if (FAILED(hr)) {
		LogHResult("PIXSetTargetWindow failed", hr);
	}
#endif
}

void PixCapture::RequestCapture(uint32_t frameCount) {
	// F11が拾えているかどうかを切り分けるため、必ずログを出す
	LogManager::GetInstance().Log("PixCapture : RequestCapture called");

	if (!isAvailable_) {
		statusMessage_ = "Capture requested but PIX is not available";
		LogManager::GetInstance().Log("PixCapture : " + statusMessage_);
		return;
	}
	if (isCapturing_) {
		LogManager::GetInstance().Log("PixCapture : already capturing, request ignored");
		return;
	}

	requestedFrames_ = (frameCount == 0) ? 1 : frameCount;
}

void PixCapture::BeginFrame() {
#if defined(USE_PIX)
	if (!isAvailable_) { return; }
	if (isCapturing_) { return; }
	if (requestedFrames_ == 0) { return; }

	const uint32_t frames = requestedFrames_;
	requestedFrames_ = 0;

	// 出力先ディレクトリを用意
	const std::wstring directory = ResolveOutputDirectory();
	std::error_code ec;
	std::filesystem::create_directories(directory, ec);
	if (ec) {
		statusMessage_ = "Failed to create output directory : " + ConvertString(directory);
		LogManager::GetInstance().Log("PixCapture : " + statusMessage_);
		return;
	}

	lastCapturePath_ = MakeCaptureFilePath();
	lastCapturePathUtf8_ = ConvertString(lastCapturePath_);

	// 次のPresentからPresentまでを1フレームとしてキャプチャ
	const HRESULT hr = PIXGpuCaptureNextFrames(lastCapturePath_.c_str(), frames);
	if (FAILED(hr)) {
		statusMessage_ = "PIXGpuCaptureNextFrames failed";
		LogHResult("PIXGpuCaptureNextFrames failed", hr);
		return;
	}

	isCapturing_ = true;
	// キャプチャ本体のフレーム数 + ファイル書き出し待ちの猶予
	remainingFrames_ = frames + kCaptureGraceFrames;
	statusMessage_ = "Capturing...";
	LogManager::GetInstance().Log("PixCapture : capture enqueued -> " + lastCapturePathUtf8_);
#endif
}

void PixCapture::EndFrame() {
#if defined(USE_PIX)
	if (!isCapturing_) { return; }

	if (remainingFrames_ > 0) {
		--remainingFrames_;
	}
	if (remainingFrames_ > 0) { return; }

	isCapturing_ = false;

	// 存在を確認する
	std::error_code ec;
	if (!std::filesystem::exists(lastCapturePath_, ec)) {
		statusMessage_ = "Capture file was not created : " + lastCapturePathUtf8_;
		LogManager::GetInstance().Log("PixCapture : " + statusMessage_);
		return;
	}

	statusMessage_ = "Saved : " + lastCapturePathUtf8_;
	LogManager::GetInstance().Log("PixCapture : " + statusMessage_);

	if (openAfterCapture_) {
		OpenCaptureFile();
	}
#endif
}

std::wstring PixCapture::FindDefaultOutputDirectory() {
	std::error_code ec;
	std::filesystem::path current = std::filesystem::current_path(ec);
	if (ec) { return L"Captures"; }

	const std::filesystem::path start = current;

	for (int i = 0; i < 8; ++i) {
		const std::filesystem::path generated = current / L"Generated";
		if (std::filesystem::is_directory(generated, ec)) {
			return (generated / L"Outputs" / L"Captures").wstring();
		}

		const std::filesystem::path parent = current.parent_path();
		if (parent.empty() || parent == current) { break; }
		current = parent;
	}

	// 見つからなければカレント直下に同じ構成で作る
	return (start / L"Generated" / L"Outputs" / L"Captures").wstring();
}

std::wstring PixCapture::ResolveOutputDirectory() const {
	// 明示的に指定されていればそちらを優先する
	if (!outputDirectory_.empty()) {
		std::error_code ec;
		const std::filesystem::path absolute = std::filesystem::absolute(outputDirectory_, ec);
		return ec ? outputDirectory_ : absolute.wstring();
	}

	return FindDefaultOutputDirectory();
}

std::string PixCapture::GetOutputDirectoryUtf8() const {
	return ConvertString(ResolveOutputDirectory());
}

void PixCapture::OpenOutputDirectory() const {
	const std::wstring directory = ResolveOutputDirectory();

	std::error_code ec;
	std::filesystem::create_directories(directory, ec);

	ShellExecuteW(nullptr, L"open", directory.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

std::wstring PixCapture::MakeCaptureFilePath() const {
	const auto now = std::chrono::system_clock::now();
	const std::time_t time = std::chrono::system_clock::to_time_t(now);
	std::tm localTime{};
	localtime_s(&localTime, &time);

	std::wostringstream oss;
	oss << L"Capture_" << std::put_time(&localTime, L"%Y%m%d_%H%M%S") << L".wpix";

	// PIXには絶対パスを渡さないと保存先が意図しない場所になる
	return (std::filesystem::path(ResolveOutputDirectory()) / oss.str()).wstring();
}

void PixCapture::OpenCaptureFile() const {
	if (lastCapturePath_.empty()) { return; }

	std::error_code ec;
	if (!std::filesystem::exists(lastCapturePath_, ec)) { return; }

	// .wpixの関連付けで開く
	ShellExecuteW(nullptr, L"open", lastCapturePath_.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

namespace GameEngine {

	class PixCapture final {
	public:
		static PixCapture& GetInstance();

		/// <summary>
		/// WinPixGpuCapturer.dll をロードする。必ずID3D12Device の生成より前に呼ぶこと。
		/// </summary>
		/// <returns>ロードに成功したか</returns>
		bool Initialize();

		/// <summary>
		/// キャプチャ対象のウィンドウを設定する
		/// </summary>
		void SetTargetWindow(HWND hwnd);

		/// <summary>
		/// キャプチャが利用可能か
		/// </summary>
		bool IsAvailable() const { return isAvailable_; }

		/// <summary>
		/// 画面左上に出るPIXのHUDの表示を切り替える。
		/// 既定では非表示。
		/// </summary>
		void SetHudVisible(bool visible);
		bool IsHudVisible() const { return isHudVisible_; }

		/// <summary>
		/// 現在キャプチャ中か
		/// </summary>
		bool IsCapturing() const { return isCapturing_; }

		/// <summary>
		/// 次のフレームからキャプチャを予約する
		/// </summary>
		/// <param name="frameCount">キャプチャするフレーム数</param>
		void RequestCapture(uint32_t frameCount = 1);

		/// <summary>
		/// フレーム開始時に呼ぶ（予約されていればキャプチャ開始）
		/// </summary>
		void BeginFrame();

		/// <summary>
		/// Present の後に呼ぶ（規定フレーム数に達したらキャプチャ終了）
		/// </summary>
		void EndFrame();

		/// <summary>
		/// キャプチャ完了後に PIX で自動的に開くか
		/// </summary>
		void SetOpenAfterCapture(bool open) { openAfterCapture_ = open; }
		bool GetOpenAfterCapture() const { return openAfterCapture_; }

		/// <summary>
		/// 出力先ディレクトリを明示的に指定する。
		/// </summary>
		void SetOutputDirectory(const std::wstring& directory) { outputDirectory_ = directory; }

		/// <summary>
		/// 実際に使用される出力先ディレクトリ
		/// </summary>
		std::string GetOutputDirectoryUtf8() const;

		/// <summary>
		/// 出力先フォルダをエクスプローラーで開く
		/// </summary>
		void OpenOutputDirectory() const;

		/// <summary>
		/// 直近に保存したキャプチャのパス
		/// </summary>
		const std::string& GetLastCapturePath() const { return lastCapturePathUtf8_; }

		/// <summary>
		/// 初期化に失敗した理由など
		/// </summary>
		const std::string& GetStatusMessage() const { return statusMessage_; }

	private:
		PixCapture() = default;
		~PixCapture() = default;
		PixCapture(const PixCapture&) = delete;
		PixCapture& operator=(const PixCapture&) = delete;

		// キャプチャDLLのロードに成功したか
		bool isAvailable_ = false;
		// キャプチャ実行中か
		bool isCapturing_ = false;
		// 完了後にPIXで開くか
		bool openAfterCapture_ = true;  
		// PIXのHUDオーバーレイを出すか
		bool isHudVisible_ = false; 

		// 予約されたキャプチャフレーム数
		uint32_t requestedFrames_ = 0;
		// 残りキャプチャフレーム数
		uint32_t remainingFrames_ = 0;  

		HWND targetWindow_ = nullptr;

		std::wstring outputDirectory_;
		std::wstring lastCapturePath_;
		std::string lastCapturePathUtf8_;
		std::string statusMessage_ = "Not initialized";

	private:
		// 出力ファイルパスを生成する
		std::wstring MakeCaptureFilePath() const;

		// 実際に使う出力先ディレクトリを解決する
		std::wstring ResolveOutputDirectory() const;

		// ディレクトリを探して返す
		static std::wstring FindDefaultOutputDirectory();

		// 保存した.wpixをPIXで開く
		void OpenCaptureFile() const;

		// キャプチャファイルの書き出しを待つ猶予フレーム数
		static constexpr uint32_t kCaptureGraceFrames = 5;		
	};
}

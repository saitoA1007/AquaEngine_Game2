#pragma once
#include<chrono>
#include<thread>

/// <summary>
/// FPSを固定化する
/// </summary>
class FrameRateController {
public:

	/// <summary>
	/// FPS固定初期化
	/// </summary>
	void InitializeFixFPS();

	/// <summary>
	/// FPS固定更新
	/// </summary>
	void UpdateFixFPS();

private:
	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;
};
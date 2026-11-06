#pragma once
#include <chrono>
#include <iostream>

namespace GameEngine {

	class FpsCounter {
	public:

		/// <summary>
		/// 初期化
		/// </summary>
		void Initialize();

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update();

	public:

		/// <summary>
		/// デルタ時間を取得する。ヒットストップなどで止まらないオブジェクトに使用可能
		/// </summary>
		/// <returns>デルタ時間（秒）</returns>
		static float GetDeltaTime() { return deltaTime_; }

		/// <summary>
		/// ゲーム用のデルタ時間を取得する。ヒットストップなどで止まるオブジェクトに使用可能
		/// </summary>
		/// <returns>デルタ時間 × タイムスケール（秒）</returns>
		static float GetGameDeltaTime() { return gameDeltaTime_; }

		/// <summary>
		/// ゲーム用のタイムスケールを設定する。0で停止、1で通常、0.5でスロー
		/// </summary>
		/// <param name="timeScale">タイムスケール（0以上）</param>
		static void SetTimeScale(float timeScale);

		/// <summary>
		/// ゲーム用のタイムスケールを取得する
		/// </summary>
		/// <returns>タイムスケール</returns>
		static float GetTimeScale() { return timeScale_; }

		/// <summary>
		/// 直近1秒間のFPSを取得する
		/// </summary>
		/// <returns>FPS</returns>
		static float GetMaxFrameCount() { return maxFrameCount_; }

		/// <summary>
		/// 現在の1秒間で経過したフレーム数を取得する
		/// </summary>
		/// <returns>フレーム数</returns>
		static int GetFrameCount() { return frameCount_; }

	private:
		// 現在の時間を保持
		std::chrono::time_point<std::chrono::high_resolution_clock> currentTime_;
		// 前の時間を保持
		std::chrono::time_point<std::chrono::high_resolution_clock> preTime_;

		// 時間の累積
		float elapsedTime_ = 0.0f;

		// 直近1秒間のFPS
		static float maxFrameCount_;
		// デルタ時間
		static float deltaTime_;
		// ゲーム用のデルタ時間。ヒットストップなどの加工をおこなう
		static float gameDeltaTime_;
		// ゲーム用のタイムスケール
		static float timeScale_;
		// fpsの累積
		static int frameCount_;
	};
}

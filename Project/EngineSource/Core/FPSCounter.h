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

		/// <summary>
		/// デルタタイムを取得
		/// </summary>
		/// <returns></returns>
		float GetDeltaTime() { return 1.0f / static_cast<float>(maxFrameCount_); }

		/// <summary>
		/// 最大フレーム数を取得
		/// </summary>
		/// <returns></returns>
		int GetMaxFrameCount() { return maxFrameCount_; }

		// 最大フレーム
		static float maxFrameCount;

		// デルタ時間
		static float deltaTime;

		// ゲーム用のデルタ時間。ヒットストップなどの加工などをおこなう
		static float gameDeltaTime;

		// fpsの累積
		static int frameCount_;

	private:
		// 現在の時間を保持
		std::chrono::time_point<std::chrono::high_resolution_clock> currentTime_;
		// 前の時間を保持
		std::chrono::time_point<std::chrono::high_resolution_clock> preTime_;
		
		// fpsの最大値
		int maxFrameCount_ = 0;
		// 時間の累積
		float elapsedTime_ = 0.0f;
	};
}
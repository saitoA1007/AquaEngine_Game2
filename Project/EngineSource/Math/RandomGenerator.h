#pragma once
#include <random>
#include"Vector3.h"

class RandomGenerator {
public:

	/// <summary>
	/// 初期化処理(シーンで1回だけ処理)
	/// </summary>
	static void Initialize();

	// 指定した型のランダム値を取得
	template <typename T>
	static T Get(T min, T max) {
		if constexpr (std::is_integral_v<T>) {
			std::uniform_int_distribution<T> dist(min, max);
			return dist(randomEngine_);
		} else if constexpr (std::is_floating_point_v<T>) {
			std::uniform_real_distribution<T> dist(min, max);
			return dist(randomEngine_);
		} else {
			static_assert(std::is_arithmetic_v<T>, "RandomGenerator::Get は数値型のみ対応しています");
		}
	}

	static Vector3 GetVector3(float min, float max);
	static Vector3 GetVector3(Vector3 min, Vector3 max);
private:

	static std::mt19937 randomEngine_;
};
#pragma once
#include"Vector2.h"
#include"Vector3.h"
#include"Quaternion.h"
#include"EaseCurve.h"

// イージングタイプ
enum class EaseType {

	// 等速

	kLinear,

	// Quadratic

	kEaseInQuad,
	kEaseOutQuad,
	kEaseInOutQuad,
	kEaseOutInQuad,

	// Cubic

	kEaseInCubic,
	kEaseOutCubic,
	kEaseInOutCubic,
	kEaseOutInCubic,

	// Quartic

	kEaseInQuart,
	kEaseOutQuart,
	kEaseInOutQuart,
	kEaseOutInQuart,

	// Quintic

	kEaseInQuint,
	kEaseOutQuint,
	kEaseInOutQuint,
	kEaseOutInQuint,

	// Sine

	kEaseInSine,
	kEaseOutSine,
	kEaseInOutSine,
	kEaseOutInSine,

	// Exponential

	kEaseInExpo,
	kEaseOutExpo,
	kEaseInOutExpo,
	kEaseOutInExpo,

	// Circular

	kEaseInCirc,
	kEaseOutCirc,
	kEaseInOutCirc,
	kEaseOutInCirc,

	// Back

	kEaseInBack,
	kEaseOutBack,
	kEaseInOutBack,
	kEaseOutInBack,

	// Elastic

	kEaseInElastic,
	kEaseOutElastic,
	kEaseInOutElastic,
	kEaseOutInElastic,

	// Bounce

	kEaseInBounce,
	kEaseOutBounce,
	kEaseInOutBounce,
	kEaseOutInBounce,

	kMaxCount
};
inline constexpr const char* EaseTypeNames[] = {
	// Linear
"Linear",

// Quadratic
"EaseInQuad",
"EaseOutQuad",
"EaseInOutQuad",
"EaseOutInQuad",

// Cubic
"EaseInCubic",
"EaseOutCubic",
"EaseInOutCubic",
"EaseOutInCubic",

// Quartic
"EaseInQuart",
"EaseOutQuart",
"EaseInOutQuart",
"EaseOutInQuart",

// Quintic
"EaseInQuint",
"EaseOutQuint",
"EaseInOutQuint",
"EaseOutInQuint",

// Sine
"EaseInSine",
"EaseOutSine",
"EaseInOutSine",
"EaseOutInSine",

// Exponential
"EaseInExpo",
"EaseOutExpo",
"EaseInOutExpo",
"EaseOutInExpo",

// Circular
"EaseInCirc",
"EaseOutCirc",
"EaseInOutCirc",
"EaseOutInCirc",

// Back
"EaseInBack",
"EaseOutBack",
"EaseInOutBack",
"EaseOutInBack",

// Elastic
"EaseInElastic",
"EaseOutElastic",
"EaseInOutElastic",
"EaseOutInElastic",

// Bounce
"EaseInBounce",
"EaseOutBounce",
"EaseInOutBounce",
"EaseOutInBounce"
};

namespace GameEngine {

	Quaternion Lerp(const Quaternion& start, const Quaternion& end, const float& t);

	// 球面線形補間
	Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t, EaseType type = EaseType::kLinear);
	Vector3 Slerp(const Vector3& start, const Vector3& end, float t, EaseType type = EaseType::kLinear);

	/// <summary>
	/// イージング関数を適応
	/// </summary>
	/// <param name="t">進行状況</param>
	/// <param name="type">使用タイプ</param>
	/// <returns></returns>
	float Apply(float t, EaseType type);

	/// <summary>
	/// 補間
	/// </summary>
	/// <typeparam name="T">任意の型</typeparam>
	/// <param name="start">開始</param>
	/// <param name="end">終了</param>
	/// <param name="t">進行状況</param>
	/// <param name="type">イージングタイプ</param>
	/// <returns></returns>
	template<typename T>
	T Lerp(const T& start, const T& end, float t, EaseType type = EaseType::kLinear) {
		float easedT = Apply(t, type);
		return T(start + (end - start) * easedT);
	}

	/// <summary>
	/// カスタムカーブを適応
	/// </summary>
	/// <param name="t">進行状況</param>
	/// <param name="curve">使用するカーブ</param>
	/// <returns></returns>
	float Apply(float t, const EaseCurve& curve);

	/// <summary>
	/// カスタムカーブで補間
	/// </summary>
	/// <typeparam name="T">任意の型</typeparam>
	/// <param name="start">開始</param>
	/// <param name="end">終了</param>
	/// <param name="t">進行状況</param>
	/// <param name="curve">使用するカーブ</param>
	/// <returns></returns>
	template<typename T>
	T Lerp(const T& start, const T& end, float t, const EaseCurve& curve) {
		float easedT = Apply(t, curve);
		return T(start + (end - start) * easedT);
	}
}


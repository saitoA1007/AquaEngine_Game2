#include "pch.h"
#include "EaseCurve.h"
#include <algorithm>

using namespace GameEngine;

namespace {
	// 0除算を防ぐための最小の時間差
	constexpr float kMinDeltaTime = 1e-6f;
}

EaseCurve::EaseCurve() {
	keys_.push_back({ 0.0f, 0.0f, 1.0f, 1.0f, TangentMode::kLinear });
	keys_.push_back({ 1.0f, 1.0f, 1.0f, 1.0f, TangentMode::kLinear });
	UpdateTangents();
}

EaseCurve::EaseCurve(const std::vector<EaseKey>& keys) {
	SetKeys(keys);
}

float EaseCurve::Evaluate(float t) const {
	if (keys_.empty()) { return t; }
	if (keys_.size() == 1) { return keys_[0].value; }

	// 範囲外は端の値
	if (t <= keys_.front().time) { return keys_.front().value; }
	if (t >= keys_.back().time) { return keys_.back().value; }

	// tを含む区間を探す
	auto it = std::upper_bound(keys_.begin(), keys_.end(), t,
		[](float v, const EaseKey& k) { return v < k.time; });
	const EaseKey& k1 = *it;
	const EaseKey& k0 = *(it - 1);

	float dt = k1.time - k0.time;
	if (dt <= kMinDeltaTime) { return k1.value; }

	// 区間内の正規化した位置
	float u = (t - k0.time) / dt;
	float u2 = u * u;
	float u3 = u2 * u;

	// エルミート基底関数
	float h00 = 2.0f * u3 - 3.0f * u2 + 1.0f;
	float h10 = u3 - 2.0f * u2 + u;
	float h01 = -2.0f * u3 + 3.0f * u2;
	float h11 = u3 - u2;

	// 傾きはdValue/dTimeなので区間の長さを掛ける
	return h00 * k0.value + h10 * dt * k0.outTangent
		+ h01 * k1.value + h11 * dt * k1.inTangent;
}

size_t EaseCurve::AddKey(const EaseKey& key) {
	// 時間順を保つ位置に挿入
	auto it = std::upper_bound(keys_.begin(), keys_.end(), key.time,
		[](float v, const EaseKey& k) { return v < k.time; });
	size_t index = static_cast<size_t>(std::distance(keys_.begin(), it));
	keys_.insert(it, key);
	UpdateTangents();
	return index;
}

void EaseCurve::RemoveKey(size_t index) {
	if (keys_.size() <= 2 || index >= keys_.size()) { return; }
	keys_.erase(keys_.begin() + index);
	UpdateTangents();
}

void EaseCurve::SetKey(size_t index, const EaseKey& key) {
	if (index >= keys_.size()) { return; }
	keys_[index] = key;
	SortKeys();
	UpdateTangents();
}

void EaseCurve::SetKeys(const std::vector<EaseKey>& keys) {
	keys_ = keys;
	// キーが足りない場合は直線にする
	if (keys_.size() < 2) {
		keys_.clear();
		keys_.push_back({ 0.0f, 0.0f, 1.0f, 1.0f, TangentMode::kLinear });
		keys_.push_back({ 1.0f, 1.0f, 1.0f, 1.0f, TangentMode::kLinear });
	}
	SortKeys();
	UpdateTangents();
}

void EaseCurve::SortKeys() {
	std::stable_sort(keys_.begin(), keys_.end(),
		[](const EaseKey& a, const EaseKey& b) { return a.time < b.time; });
}

void EaseCurve::UpdateTangents() {
	const size_t n = keys_.size();
	for (size_t i = 0; i < n; ++i) {
		EaseKey& k = keys_[i];

		switch (k.mode) {
		case TangentMode::kFree:
			// 入りと出を揃える
			k.inTangent = k.outTangent;
			break;

		case TangentMode::kBroken:
			// 手動設定のまま
			break;

		case TangentMode::kLinear:
			// 入り側は前のキー、出り側は次のキーへの傾き
			if (i > 0) {
				const EaseKey& prev = keys_[i - 1];
				k.inTangent = (k.value - prev.value) / std::max(k.time - prev.time, kMinDeltaTime);
			}
			if (i + 1 < n) {
				const EaseKey& next = keys_[i + 1];
				k.outTangent = (next.value - k.value) / std::max(next.time - k.time, kMinDeltaTime);
			}
			// 端のキーは反対側と同じ傾きにする
			if (i == 0) { k.inTangent = k.outTangent; }
			if (i + 1 == n) { k.outTangent = k.inTangent; }
			break;

		case TangentMode::kAuto:
		default:
		{
			// 前後のキーを結ぶ傾き
			const EaseKey& prev = keys_[i > 0 ? i - 1 : i];
			const EaseKey& next = keys_[i + 1 < n ? i + 1 : i];
			float slope = (next.value - prev.value) / std::max(next.time - prev.time, kMinDeltaTime);
			k.inTangent = slope;
			k.outTangent = slope;
			break;
		}
		}
	}
}


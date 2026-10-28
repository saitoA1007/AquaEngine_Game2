#pragma once
#include <vector>
#include <cstddef>

namespace GameEngine {

	// 接線の決め方
	enum class TangentMode {
		kAuto,   // 前後のキーから自動で滑らかに
		kFree,   // 入りと出の傾きは同じで手動設定
		kBroken, // 入りと出の傾きを個別で手動設定
		kLinear, // 隣のキーへ直線

		kMaxCount
	};
	inline constexpr const char* TangentModeNames[] = {
		"Auto",
		"Free",
		"Broken",
		"Linear"
	};

	// 制御点
	struct EaseKey {
		float time = 0.0f;       // 進行状況
		float value = 0.0f;      // 出力値
		float inTangent = 0.0f;  // 入り側の傾き
		float outTangent = 0.0f; // 出り側の傾き
		TangentMode mode = TangentMode::kAuto;
	};

	/// <summary>
	/// 制御点と接線で自由に形を決められるイージングカーブ
	/// </summary>
	class EaseCurve {
	public:
		EaseCurve();
		explicit EaseCurve(const std::vector<EaseKey>& keys);

		/// <summary>
		/// カーブを評価
		/// </summary>
		/// <param name="t">進行状況</param>
		/// <returns>補間後の値</returns>
		float Evaluate(float t) const;

		/// <summary>
		/// キーを追加
		/// </summary>
		/// <returns>追加したキーのインデックス</returns>
		size_t AddKey(const EaseKey& key);

		/// <summary>
		/// キーを削除
		/// </summary>
		void RemoveKey(size_t index);

		/// <summary>
		/// キーを変更
		/// </summary>
		void SetKey(size_t index, const EaseKey& key);

		/// <summary>
		/// 全てのキーを置き換える
		/// </summary>
		void SetKeys(const std::vector<EaseKey>& keys);

		const std::vector<EaseKey>& GetKeys() const { return keys_; }

	private:
		std::vector<EaseKey> keys_;

	private:
		void SortKeys();
		void UpdateTangents();
	};
}

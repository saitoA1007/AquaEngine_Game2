#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <json.hpp>
#include "Vector3.h"

namespace GameEngine {

	/// <summary>
	/// トラックのパーティクル発生方法
	/// </summary>
	enum class EffectEmitMode {
		kContinuous, // 開始時間から終了時間まで連続で発生させる
		kBurst,		 // 開始時間に1回だけ発生させる
	};

	/// <summary>
	/// エフェクトを構成する1本のトラックのデータ
	/// </summary>
	struct EffectTrackData {
		// エディター上の表示名
		std::string name = "Track";
		// 使用するパーティクルの名前
		std::string particleName;
		// 使用するモデルの名前
		std::string modelName;
		// 最大パーティクル数
		uint32_t maxNum = 256;
		// 発生方法
		EffectEmitMode emitMode = EffectEmitMode::kContinuous;
		// 開始時間（秒）
		float startTime = 0.0f;
		// 発生させる時間
		float duration = 1.0f;
		// エフェクトの位置からのオフセット
		Vector3 offset = { 0.0f,0.0f,0.0f };

		// 終了時間を取得
		float GetEndTime() const {
			return emitMode == EffectEmitMode::kBurst ? startTime : startTime + duration;
		}
	};

	/// <summary>
	/// jsonに保存するエフェクトの定義データ
	/// </summary>
	struct EffectAsset {
		// エフェクトの名前
		std::string name;
		// エフェクト全体の長さ
		float duration = 2.0f;
		// ループ再生するか
		bool isLoop = false;
		// トラック
		std::vector<EffectTrackData> tracks;

		/// <summary>
		/// jsonに変換する
		/// </summary>
		/// <returns></returns>
		nlohmann::json ToJson() const;

		/// <summary>
		/// jsonから読み込む。存在しない項目はデフォルト値になる
		/// </summary>
		/// <param name="name">エフェクトの名前</param>
		/// <param name="root">読み込むjson</param>
		/// <returns></returns>
		static EffectAsset FromJson(const std::string& name, const nlohmann::json& root);

		/// <summary>
		/// ファイルに保存する
		/// </summary>
		void Save() const;

		/// <summary>
		/// ファイルから読み込む
		/// </summary>
		/// <param name="name">エフェクトの名前</param>
		/// <param name="outAsset">読み込んだデータ</param>
		/// <returns>読み込めたか</returns>
		static bool Load(const std::string& name, EffectAsset& outAsset);

		// 保存先のディレクトリ
		static inline const std::string kDirectoryPath = "Resources/Json/Effects/";
	};
}

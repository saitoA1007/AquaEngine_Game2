#pragma once
#include <vector>
#include <memory>
#include "EffectModule.h"

namespace GameEngine {

	// 前方宣言
	class ModelManager;

	/// <summary>
	/// 1つのエフェクトオブジェクト
	/// </summary>
	class EffectObject {
	public:
		EffectObject(const EffectAsset& asset, TextureManager* textureManager, ModelManager* modelManager);
		~EffectObject() = default;

		// 更新処理
		void Update();

		/// <summary>
		/// 経過時間を指定して更新する
		/// </summary>
		/// <param name="deltaTime">経過時間</param>
		void Update(float deltaTime);

		/// <summary>
		/// 指定した時間まで早送りする。現在より前の時間なら最初から再生し直す
		/// </summary>
		/// <param name="targetTime">移動先の時間</param>
		/// <param name="step">1回の更新で進める時間</param>
		void Seek(float targetTime, float step = 1.0f / 60.0f);

		// 描画処理
		void Draw();

	public:

		/// <summary>
		/// 最初から再生する
		/// </summary>
		/// <param name="pos">再生する位置</param>
		void Play(const Vector3& pos);

		/// <summary>
		/// 発生を止める
		/// </summary>
		void Stop();

		/// <summary>
		/// 全てのパーティクルを消去して即座に再生終了にする
		/// </summary>
		void StopImmediate();

		// 再生中か
		bool IsPlaying() const { return isPlaying_; }

		// 位置を設定
		void SetPosition(const Vector3& pos) { position_ = pos; }
		const Vector3& GetPosition() const { return position_; }

		// 現在の再生時間
		float GetTime() const { return time_; }

		// エフェクトの名前
		const std::string& GetName() const { return name_; }

		// モジュールを取得
		const std::vector<std::unique_ptr<EffectModule>>& GetModules() const { return modules_; }

		/// <summary>
		/// 長さ、ループ、各トラックのタイミングなどの項目を反映する
		/// </summary>
		/// <param name="asset">生成時と同じ構成のエフェクト</param>
		void ApplyTiming(const EffectAsset& asset);

	private:
		// エフェクトの名前
		std::string name_;

		// エフェクト全体の長さ
		float duration_ = 0.0f;

		// ループ再生するか
		bool isLoop_ = false;

		// トラックごとのモジュール
		std::vector<std::unique_ptr<EffectModule>> modules_;

		// モジュールに対応するエフェクトのトラックの番号
		std::vector<size_t> trackIndices_;

		// 位置
		Vector3 position_ = { 0.0f,0.0f,0.0f };

		// 現在の再生時間
		float time_ = 0.0f;

		// 再生中か
		bool isPlaying_ = false;

		// 発生処理が終了したか
		bool isEmitFinished_ = false;

	private:

		// [prevTime, time) の区間の発生処理
		void UpdateEmission(float prevTime, float time);

		// 生きているパーティクルが残っているか
		bool HasAliveParticles() const;
	};
}

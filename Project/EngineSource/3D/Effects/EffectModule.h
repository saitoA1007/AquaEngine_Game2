#pragma once
#include <memory>
#include "ParticleBehavior.h"
#include "EffectAsset.h"

namespace GameEngine {

	/// <summary>
	/// エフェクトの1トラック分を再生するクラス
	/// </summary>
	class EffectModule {
	public:
		EffectModule(const EffectTrackData& data, TextureManager* textureManager, Model* model);
		~EffectModule() = default;

		/// <summary>
		/// 発生の制御をする
		/// </summary>
		/// <param name="prevTime">前フレームのエフェクトの時間</param>
		/// <param name="time">現在のエフェクトの時間</param>
		/// <param name="basePos">エフェクトの位置</param>
		void UpdateEmission(float prevTime, float time, const Vector3& basePos);

		/// <summary>
		/// パーティクルの更新処理
		/// </summary>
		/// <param name="deltaTime">経過時間（秒）</param>
		void Update(float deltaTime);

		/// <summary>
		/// 描画処理
		/// </summary>
		void Draw();

		/// <summary>
		/// 連続発生を止める。発生済みのパーティクルは寿命まで更新される
		/// </summary>
		void StopEmission();

		/// <summary>
		/// 全てのパーティクルを消去する
		/// </summary>
		void Reset();

		// 生きているパーティクルが残っているか
		bool HasAliveParticles() const { return particle_->HasAliveParticles(); }

		// トラックのデータを取得
		const EffectTrackData& GetTrackData() const { return data_; }

		// トラックのデータを設定
		void SetTrackData(const EffectTrackData& data);

		// パーティクルを取得
		ParticleBehavior* GetParticle() const { return particle_.get(); }

	private:
		// トラックのデータ
		EffectTrackData data_;

		// パーティクル
		std::unique_ptr<ParticleBehavior> particle_;

	private:

		// 発生方法をパーティクルに反映する
		void ApplyEmitMode();
	};
}

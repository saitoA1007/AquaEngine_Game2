#pragma once
#include <memory>
#include "IGameObject.h"
#include "Collider.h"
#include "Model.h"
#include "WorldTransform.h"
#include "IceMaterial.h"
#include "DebugParameter.h"
#include "FractureDamageController.h"

namespace GameEngine {

	/// <summary>
	/// 破壊オブジェクト
	/// </summary>
	class DestructibleObject : public IGameObject {
	public:
		DestructibleObject(std::string name, Model* model, uint32_t colliderId, uint32_t colliderAttribute);
		~DestructibleObject() = default;

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		// 描画処理
		void Draw() override;

	public:

		// 静的な破片を元の位置へ戻し、無傷の状態へ復元するアニメーションを開始する
		void Reassemble() { damageController_.BeginReassembly(); }

		// 元に戻るアニメーションを再生中か
		bool IsReassembling() const { return damageController_.IsReassembling(); }

		// 破片を全て消し、すぐに無傷の状態へ戻す
		void ResetToIntact() { damageController_.ResetToIntact(); }

		// 最下層のチャンクをアンカーとして切り離さないようにするか
		void SetUseAnchor(bool isUse) { damageController_.SetUseAnchor(isUse); }

		// 破片が跳ねる地面の高さ(ワールド座標)を設定する
		void SetGroundHeight(float worldY) { damageController_.SetGroundHeight(worldY); }

		/// <summary>
		/// ワールド座標で指定した位置にダメージを与える
		/// </summary>
		/// <param name="worldImpactPos">当たった位置</param>
		/// <param name="worldImpactDirection">当たった方向</param>
		/// <param name="damageAmount">与えるダメージ</param>
		/// <param name="craterRadius">ダメージを与える範囲</param>
		void ApplyDamage(const Vector3& worldImpactPos, const Vector3& worldImpactDirection, float damageAmount, float craterRadius);

		// チャンクの破壊された割合
		float GetDestroyedRatio() const { return damageController_.GetDestroyedRatio(); }

		// 当たり判定の設定
		void SetIsColliderActive(bool isActive) {
			collider_.SetActive(isActive);
		}

		// 当たり判定のコールバック関数
		void OnCollisionEnter(const GameEngine::CollisionResult& result);

	public:

		// ワールド行列
		WorldTransform worldTransform_;

		// 当たり判定のサイズ
		Vector3 colliderSize_ = { 2.5f, 2.5f, 2.5f };

		// 与えるダメージ
		float damageAmount_ = 2.0f;
		// ダメージを与える範囲
		float craterRadius_ = 2.0f;
		// 切る数
		int planeCount_ = 8;

		// 氷のマテリアル
		IceMaterial iceMaterial_;

	private:
		// 名前
		std::string name_ = "noName";

		// モデル
		Model* model_ = nullptr;

		// aabbの当たり判定
		AABBCollider collider_;

		// コライダー生成時に渡す識別情報
		uint32_t colliderId_ = 0;
		uint32_t colliderAttribute_ = 0;

		// ダメージ判定、破砕伝播、爆発、ひび割れ物理
		FractureDamageController damageController_;

		// パラメータ機能
		std::unique_ptr<DebugParameter> debugParameter_;		
	};
}

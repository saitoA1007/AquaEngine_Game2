#pragma once
#include <cstdint>
#include <functional>
#include "Vector3.h"
#include "CollisionResult.h"
#include "ColliderInfo.h"
#include "MyMath.h"

namespace GameEngine {

	// 前方宣言
	class CollisionManager;

	/// <summary>
	/// 当たり判定が持つ基盤の要素
	/// </summary>
	class Collider {
	public:
		Collider();
		virtual ~Collider();

		// 当たり判定管理クラスを取得
		static void StaticInitialize(CollisionManager* manager) { collisionManager_ = manager; }

		// 衝突形状を取得
		virtual CollisionData GetCollisionData() const = 0;

		// ワールド座標を取得
		Vector3 GetWorldPosition() const { return worldPosition_; }
		void SetWorldPosition(const Vector3& position) { worldPosition_ = position; }

		// 触れた瞬間を取得するコールバック関数を登録する
		void SetOnCollisionEnterCallback(std::function<void(const CollisionResult&)> callback) {
			onCollisionEnterCallback_ = callback;
		}

		// 衝突開始時に呼ばれる関数
		void OnCollisionEnter(const CollisionResult& result) {
			if (onCollisionEnterCallback_) {
				onCollisionEnterCallback_(result);
			}
		}

		// コールバック関数を登録する
		void SetOnCollisionCallback(std::function<void(const CollisionResult&)> callback) {
			onCollisionCallback_ = callback;
		}

		// 衝突時に呼ばれる関数
		void OnCollision(const CollisionResult& result) {
			// コールバックを実行する
			if (onCollisionCallback_) {
				onCollisionCallback_(result);
			}
		}

		// 衝突属性を取得
		uint32_t GetCollisionAttribute() const { return collisionAttribute_; }
		// 衝突属性を設定
		void SetCollisionAttribute(const uint32_t& collisionAttribute) { collisionAttribute_ = collisionAttribute; }

		// 衝突マスク
		uint32_t GetCollisionMask() const { return collisionMask_; }
		// 衝突マスク
		void SetCollisionMask(const uint32_t& collisionMask) { collisionMask_ = collisionMask; }

		// 有効状態
		bool IsActive() const { return isActive_; }
		void SetActive(const bool& isActive) { isActive_ = isActive; }

		// ユーザーデータを設定
		void SetUserData(const UserData& data) { userData_ = data; }
		const UserData& GetUserData() const { return userData_; }

	protected:
		// 当たり判定管理
		static CollisionManager* collisionManager_;

		// 衝突属性(自分)
		uint32_t collisionAttribute_ = 0xffffffff;
		// 衝突マスク(相手)
		uint32_t collisionMask_ = 0xffffffff;
		// 当たり判定の有効化
		bool isActive_ = true;
		// ワールド座標
		Vector3 worldPosition_ = { 0.0f, 0.0f, 0.0f };
		// コールバック関数(触れた時)
		std::function<void(const CollisionResult&)> onCollisionEnterCallback_;
		// コールバック関数(触れている間)
		std::function<void(const CollisionResult&)> onCollisionCallback_;
		// ユーザーデータ
		UserData userData_;
	};

	/// <summary>
	/// 球の当たり判定
	/// </summary>
	class SphereCollider : public Collider {
	public:

		// 球の当たり判定を登録
		CollisionData GetCollisionData() const override {
			CollisionData collisionData;
			collisionData.data = Sphere{ worldPosition_,radius_ };
			collisionData.shapeType = ShapeType::kSphere;
			return collisionData;
		}

		const float GetRadius() const { return radius_; }
		void SetRadius(const float& radius) { radius_ = radius; }

	private:
		// 半径
		float radius_;
	};

	/// <summary>
	/// AABBの当たり判定
	/// </summary>
	class AABBCollider : public Collider {
	public:

		// AABBの当たり判定を登録する
		CollisionData GetCollisionData() const override {
			CollisionData collisionData;
			Vector3 min = worldPosition_ - (size_ * anchorPoint_);
			Vector3 max = min + size_;
			collisionData.data = AABB{ min, max };
			collisionData.shapeType = ShapeType::kAABB;
			return collisionData;
		}

		// サイズを取得する
		const Vector3 GetSize() const { return size_; }
		void SetSize(const Vector3& size) { size_ = size; }

		// アンカーポイント
		const Vector3& GetAnchorPoint() const { return anchorPoint_; }
		void SetAnchorPoint(const Vector3& anchorPoint) { anchorPoint_ = anchorPoint; }

	private:
		// サイズ
		Vector3 size_;
		// アンカーポイント
		Vector3 anchorPoint_ = {0.5f,0.5f,0.5f};
	};

	/// <summary>
	/// 線分の当たり判定
	/// </summary>
	class SegmentCollider : public Collider {
	public:
		// 線分の当たり判定を登録する
		CollisionData GetCollisionData() const override {
			CollisionData collisionData;
			Vector3 pos = worldPosition_;
			collisionData.data = Segment{ pos,diff_ };
			collisionData.shapeType = ShapeType::kSegment;
			return collisionData;
		}

		// 線の方向
		const Vector3 GetDiff() const { return diff_; }
		void SetDiff(const Vector3& diff) { diff_ = diff; }

	private:
		Vector3 diff_;
	};

	/// <summary>
	/// OBBの当たり判定
	/// </summary>
	class OBBCollider : public Collider {
	public:
		// obbの当たり判定を登録する
		CollisionData GetCollisionData() const override {
			CollisionData collisionData;
			OBB tmpOBB;

			// ローカルオフセットを計算
			Vector3 localOffset = {
				(0.5f - anchor_.x) * (size_.x * 2.0f),
				(0.5f - anchor_.y) * (size_.y * 2.0f),
				(0.5f - anchor_.z) * (size_.z * 2.0f)
			};

			// 各座標軸を考慮してワールド空間のオフセットに変換
			Vector3 worldOffset =
				orientations_[0] * localOffset.x +
				orientations_[1] * localOffset.y +
				orientations_[2] * localOffset.z;


			tmpOBB.center = worldPosition_ + worldOffset;
			tmpOBB.size = size_;
			std::memcpy(tmpOBB.orientations, orientations_, sizeof(Vector3) * 3);
			collisionData.data = tmpOBB;
			collisionData.shapeType = ShapeType::kOBB;
			return collisionData;
		}

		// 座標軸
		void SetOrientations(Vector3 orientations[3]) { std::memcpy(orientations_, orientations, sizeof(Vector3) * 3); }
		const Vector3* GetOrientations() const { return orientations_; }

		// オイラー角から座標軸を取得する
		void UpdateOrientationsFromRotate(const Vector3& rotate) {
			Matrix4x4 rotationMatrix = Matrix4x4::MakeRotateXMatrix(rotate.x) * (Matrix4x4::MakeRotateYMatrix(rotate.y) * Matrix4x4::MakeRotateZMatrix(rotate.z));
			orientations_[0] = { rotationMatrix.m[0][0], rotationMatrix.m[0][1], rotationMatrix.m[0][2] };
			orientations_[1] = { rotationMatrix.m[1][0], rotationMatrix.m[1][1], rotationMatrix.m[1][2] };
			orientations_[2] = { rotationMatrix.m[2][0], rotationMatrix.m[2][1], rotationMatrix.m[2][2] };
			// 正規化
			orientations_[0].Normalize();
			orientations_[1].Normalize();
			orientations_[2].Normalize();
		}

		// サイズ
		void SetSize(const Vector3& size) { size_ = size; }
		const Vector3& GetSize() const { return size_; }

		// アンカーポイント
		void SetAnchor(const Vector3& anchor) { anchor_ = anchor; }
		const Vector3& GetAnchor() const { return anchor_; }

	private:
		// 座標軸
		Vector3 orientations_[3];
		// 座標軸方向の長さの半分。中心から面までの距離
		Vector3 size_;
		// アンカーポイント
		Vector3 anchor_ = { 0.5f, 0.5f, 0.5f };
	};
}
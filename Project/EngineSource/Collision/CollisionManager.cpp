#include "pch.h"
#include "CollisionManager.h"
#include "CollisionVisitor.h"
#include "Collider.h"
#include "DebugRenderer.h"
using namespace GameEngine;

CollisionManager::~CollisionManager() {
	// Colliderのデストラクタが呼ばれても安全なようにnullを入れる
	Collider::StaticInitialize(nullptr);
	colliders_.clear();
}

void CollisionManager::CheckAllCollisions() {

	// 現フレームの衝突リストをクリア
	currentCollisions_.clear();

	// 何も無ければ早期リターン
	if (colliders_.empty()) {
		// 前フレームの情報も更新
		preCollisions_.clear();
		return;
	}

	// リスト内のペアを総当たり
	std::list<Collider*>::iterator itrA = colliders_.begin();
	for (; itrA != colliders_.end(); ++itrA) {
		// コライダーAを取得
		Collider* colliderA = *itrA;

		// 無効なコライダーはスキップする
		if (!colliderA->IsActive()) { continue; }

		// イテレーターBはイテレータAの次の要素から回す
		std::list<Collider*>::iterator itrB = itrA;
		itrB++;

		for (; itrB != colliders_.end(); ++itrB) {
			// コライダーBを取得
			Collider* colliderB = *itrB;

			// ペアの当たり判定
			CheckCollisionPair(colliderA, colliderB);
		}
	}

	// 現在の当たり判定内容を保存
	preCollisions_ = currentCollisions_;
}

void  CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB) {

	// 衝突フィルタリング
	if (!IsActiveCollision(colliderA, colliderB)) {
		return;
	}

	// 形状を取得
	CollisionData typeA = colliderA->GetCollisionData();
	CollisionData typeB = colliderB->GetCollisionData();

	// 各形状に応じた当たり判定を取得
	CollisionResult result = CheckCollisionData(typeA, typeB);

	if (result.isHit) {

		// 現在の衝突ペアを記録
		auto collisionPair = MakeCollisionPair(colliderA, colliderB);
		currentCollisions_.insert(collisionPair);

		// 前フレームで衝突していたか確認
		bool wasHit = WasCollidingLastFrame(colliderA, colliderB);

		result.userData = colliderB->GetUserData();
		// 反対側の法線情報を渡す
		CollisionResult resultB = result;
		resultB.contactNormal = result.contactNormal * -1.0f;
		resultB.userData = colliderA->GetUserData();

		// コライダーの衝突時コールバックを呼び出す
		if (wasHit) {
			// 衝突している間に呼び出す
			colliderA->OnCollision(result);
			colliderB->OnCollision(resultB);
		} else {
			// 衝突している間に呼び出す
			colliderA->OnCollision(result);
			colliderB->OnCollision(resultB);
			// 衝突した瞬間に呼び出す
			colliderA->OnCollisionEnter(result);
			colliderB->OnCollisionEnter(resultB);
		}
	}
}

void CollisionManager::DebugDraw(DebugRenderer* debugRenderer) {
#ifdef USE_IMGUI
	if (colliders_.empty() || !debugRenderer->IsEnabled()) { return; }
	for (auto collider : colliders_) {
		// 無効なコライダーは描画しない
		if (!collider->IsActive()) { continue; }
		CollisionData type = collider->GetCollisionData();

		switch (type.shapeType)
		{
		case GameEngine::kSphere: {
			if (const Sphere* sphere = type.Get<Sphere>()) {
				debugRenderer->AddSphere(*sphere);
			}
			break;
		}
		case GameEngine::kAABB: {
			if (const AABB* aabb = type.Get<AABB>()) {
				debugRenderer->AddAABB(*aabb);
			}
			break;
		}
		case GameEngine::kOBB: {
			if (const OBB* obb = type.Get<OBB>()) {
				debugRenderer->AddOBB(*obb);
			}
			break;
		}
		case GameEngine::kSegment: {
			if (const Segment* segment = type.Get<Segment>()) {
				debugRenderer->AddRay(*segment);
			}
			break;
		}
		case GameEngine::kMaxCount:
		default:
			break;
		}		
	}
#endif
}

bool CollisionManager::IsActiveCollision(Collider* a, Collider* b) {
	// どちらかが無効なら衝突しない
	if (!a->IsActive() || !b->IsActive()) {
		return false;
	}
	return (a->GetCollisionAttribute() & b->GetCollisionMask()) != 0 && (b->GetCollisionAttribute() & a->GetCollisionMask()) != 0;
}

CollisionResult CollisionManager::CheckCollisionData(const CollisionData& dataA, const CollisionData& dataB) {
	// 各形状の当たり判定を取得する
	return std::visit(CollisionVisitor{}, dataA.data, dataB.data);
}

std::pair<Collider*, Collider*> CollisionManager::MakeCollisionPair(Collider* a, Collider* b) {
	// ポインタの値で順序を固定する
	if (a < b) {
		return { a, b };
	} else {
		return { b, a };
	}
}

bool CollisionManager::WasCollidingLastFrame(Collider* a, Collider* b) {
	auto collisionPair = MakeCollisionPair(a, b);
	return preCollisions_.find(collisionPair) != preCollisions_.end();
}

void CollisionManager::RemoveCollider(Collider* collider) {
	// リストからコライダーを削除
	colliders_.remove(collider);

	// 前フレームの衝突履歴（preCollisions_）から、このコライダーが関わっているペアを探して削除する
	for (auto it = preCollisions_.begin(); it != preCollisions_.end(); ) {
		if (it->first == collider || it->second == collider) {
			it = preCollisions_.erase(it); // 見つけたら消す
		} else {
			++it; // 次を見る
		}
	}
}
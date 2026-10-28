#include "pch.h"
#include "Collider.h"
#include "CollisionManager.h"

using namespace GameEngine;

CollisionManager* Collider::collisionManager_ = nullptr;

Collider::Collider() {
	// 登録する
	if (collisionManager_) {
		collisionManager_->AddCollider(this);
	}
}

Collider::~Collider() {
	// 登録を解除する
	if (collisionManager_) {
		collisionManager_->RemoveCollider(this);
	}
}
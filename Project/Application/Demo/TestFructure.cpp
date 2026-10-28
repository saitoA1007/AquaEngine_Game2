#include "pch.h"
#include "TestFructure.h"
#include "Application/CollisionConfig.h"
using namespace GameEngine;

TestFructure::TestFructure(GameEngine::Model* model) : destructibleObject_("TestFructure", model, static_cast<uint32_t>(CollisionTypeID::kPlayer), kCollisionAttributePlayer) {

	// 当たり判定を設定
	testCollider_.SetRadius(1.0f);
	testCollider_.SetWorldPosition(testPos_);
	testCollider_.SetCollisionAttribute(kCollisionAttributeEnemy);
	testCollider_.SetCollisionMask(~kCollisionAttributeEnemy);

	// パラメータ機能
	debugParame_.Register("Radius", radius_, 0, "TestCollider");
	debugParame_.Register("Pos", testPos_, 0, "TestCollider");
	debugParame_.Apply();
}

void TestFructure::Initialize() {

}

void TestFructure::Update() {
	debugParame_.ApplyIfDirty();

	// 当たり判定を更新
	testCollider_.SetWorldPosition(testPos_);
	testCollider_.SetRadius(radius_);

	// 破片の更新
	destructibleObject_.Update();
}

void TestFructure::Draw() {
	// 破片の描画
	destructibleObject_.Draw();
}
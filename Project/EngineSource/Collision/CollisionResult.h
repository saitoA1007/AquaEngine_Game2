#pragma once
#include "Vector3.h"
#include "UserData.h"

namespace GameEngine {

	struct CollisionResult {
		bool isHit = false; // 衝突判定
		Vector3 contactPosition = { 0.0f,0.0f,0.0f }; // 接触位置
		Vector3 contactNormal = {0.0f,0.0f,0.0f}; // 接触法線
		float penetrationDepth = 0.0f; // 侵入深度

		// ユーザーが持つデータ
		UserData userData;
	};
}
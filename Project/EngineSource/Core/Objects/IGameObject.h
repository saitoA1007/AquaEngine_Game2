#pragma once
#include <cstdint>
#include "RenderQueue.h"

namespace GameEngine {

	/// <summary>
	/// ゲームオブジェクトの基底クラス
	/// </summary>
	class IGameObject {
	public:
		virtual ~IGameObject() = default;

		// 静的初期化
		static void StaticInitialize(RenderQueue* renderQueue) {
			renderQueue_ = renderQueue;
		}

		virtual void Initialize() {};
		virtual void Update() {};
		virtual void Draw() {};

		virtual void DebugUpdate() {};

	public:

		// 更新処理順
		int32_t GetUpdateOrder() const { return updateOrder_; }
		void SetUpdateOrder(int32_t order) { updateOrder_ = order; }

		// 有効状態
		bool IsActive() const { return isActive_; }
		virtual void SetActive(bool active) { isActive_ = active; }

		// 削除管理
		bool IsDead() const { return isDead_; }
		virtual void Destroy() { isDead_ = true; }

	protected:
		// 描画機能
		static RenderQueue* renderQueue_;

		// 更新処理順
		int32_t updateOrder_ = 1;
		// 有効状態
		bool isActive_ = true;
		// 削除管理
		bool isDead_ = false;
	};
}
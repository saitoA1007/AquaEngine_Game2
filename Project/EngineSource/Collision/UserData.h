#pragma once
#include <cstdint>

namespace GameEngine {

	// 基底クラス
	class IGameObject;

	// 受け渡すデータ
	struct UserData {
		// オブジェクトを識別するID
		uint32_t typeID = 0;
		// 基底クラスポインタ
		IGameObject* object = nullptr;

		// 型安全なキャストヘルパー
		template<typename T>
		T* As() const {
			// キャストが間違えていればnullptrを返す
			return dynamic_cast<T*>(object);
		}

		// 有効化判断する
		bool IsActive() const { return object != nullptr; }
	};
}
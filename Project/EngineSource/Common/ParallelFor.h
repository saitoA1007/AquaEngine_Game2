#pragma once
#include <functional>

namespace GameEngine {

	/// <summary>
	/// 0からcount-1の処理を複数スレッドで分担して実行する
	/// </summary>
	void ParallelFor(size_t count, const std::function<void(size_t)>& func);
}

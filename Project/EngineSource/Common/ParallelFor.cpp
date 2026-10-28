#include "pch.h"
#include "ParallelFor.h"

namespace GameEngine {

	void ParallelFor(size_t count, const std::function<void(size_t)>& func) {
		if (count == 0) { return; }

		const size_t hw = std::max<size_t>(1, std::thread::hardware_concurrency());
		const size_t workerCount = std::min(count, hw);
		std::atomic<size_t> next{ 0 };

		auto worker = [&]() {
			CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			for (size_t i = next.fetch_add(1); i < count; i = next.fetch_add(1)) {
				func(i);   // 例外はfunc内で捕まえる
			}
			CoUninitialize();
			};

		std::vector<std::thread> threads;
		threads.reserve(workerCount);
		for (size_t i = 0; i < workerCount; ++i) {
			threads.emplace_back(worker);
		}
		for (auto& t : threads) {
			t.join();
		}
	}
}

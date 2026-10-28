#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "FractureInstance.h"

namespace GameEngine {

	/// <summary>
	/// 無傷、事前分割のまま切り離された破片、ランタイムカットされた破片、付着したまま動的に凹んだチャンクの状態を保持する。
	/// </summary>
	class FractureBreakState {
	public:
		FractureInstance& Intact() { return intact_; }
		const FractureInstance& Intact() const { return intact_; }
		bool HasIntact() const { return intact_.HasInstances(); }

		// 新しいマクロ破片バッチを追加して返す。呼び出し側でInitialize()する
		FractureInstance& AddMacroDebrisBatch() {
			macroDebrisBatches_.push_back(std::make_unique<FractureInstance>());
			return *macroDebrisBatches_.back();
		}
		std::vector<std::unique_ptr<FractureInstance>>& MacroDebrisBatches() { return macroDebrisBatches_; }
		const std::vector<std::unique_ptr<FractureInstance>>& MacroDebrisBatches() const { return macroDebrisBatches_; }
		bool HasMacroDebris() const { return !macroDebrisBatches_.empty(); }
		// 古いバッチから間引いて、上限バッチ数以内に収める
		void TrimMacroDebrisBatches(size_t maxBatches) {
			while (macroDebrisBatches_.size() > maxBatches) {
				macroDebrisBatches_.erase(macroDebrisBatches_.begin());
			}
		}

		// 新しいマイクロ破片バッチを追加して返す。呼び出し側でApplyRuntimeCut()する
		FractureInstance& AddMicroDebrisBatch() {
			microDebrisBatches_.push_back(std::make_unique<FractureInstance>());
			return *microDebrisBatches_.back();
		}
		std::vector<std::unique_ptr<FractureInstance>>& MicroDebrisBatches() { return microDebrisBatches_; }
		const std::vector<std::unique_ptr<FractureInstance>>& MicroDebrisBatches() const { return microDebrisBatches_; }
		bool HasMicroDebris() const { return !microDebrisBatches_.empty(); }
		// 古いバッチから間引いて、上限バッチ数以内に収める
		void TrimMicroDebrisBatches(size_t maxBatches) {
			while (microDebrisBatches_.size() > maxBatches) {
				microDebrisBatches_.erase(microDebrisBatches_.begin());
			}
		}

		// 切り離されずに付着したまま、動的にカットされて凹んだチャンク
		FractureInstance& GetOrCreateDentedChunk(uint32_t chunkId) {
			auto it = dentedChunks_.find(chunkId);
			if (it == dentedChunks_.end()) {
				it = dentedChunks_.emplace(chunkId, std::make_unique<FractureInstance>()).first;
			}
			return *it->second;
		}
		void RemoveDentedChunk(uint32_t chunkId) { dentedChunks_.erase(chunkId); }
		bool HasDentedChunk(uint32_t chunkId) const { return dentedChunks_.count(chunkId) != 0; }
		bool HasDentedChunks() const { return !dentedChunks_.empty(); }
		std::unordered_map<uint32_t, std::unique_ptr<FractureInstance>>& DentedChunks() { return dentedChunks_; }
		const std::unordered_map<uint32_t, std::unique_ptr<FractureInstance>>& DentedChunks() const { return dentedChunks_; }

	private:
		// 元の静的チャンク
		FractureInstance intact_;
		// 事前分割のまま切り離されて落ちる破片。破壊イベントごとに1バッチ追加される
		std::vector<std::unique_ptr<FractureInstance>> macroDebrisBatches_;
		// ランタイムカットされた破片。破壊イベントごとに1バッチ追加される
		std::vector<std::unique_ptr<FractureInstance>> microDebrisBatches_;
		// 切り離されずに付着したまま凹んでいるチャンク。物理は掛けず初期位置に固定して描画する
		std::unordered_map<uint32_t, std::unique_ptr<FractureInstance>> dentedChunks_;
	};
}

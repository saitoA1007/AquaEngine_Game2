#include "pch.h"
#define NOMINMAX
#include <algorithm>
#include <queue>
#include "FractureDamageController.h"
#include "RandomGenerator.h"
#include "FPSCounter.h"
#include "MyMath.h"
#include "LogManager.h"
using namespace GameEngine;

void FractureDamageController::Initialize(Model* model) {
	model_ = model;

	const auto& fractureChunks = model_->GetFractureChunks();
	// 破壊データを持たないモデルだった場合、飛ばす
	if (fractureChunks.empty()) {
		return;
	}
	groupName_ = fractureChunks.begin()->first;
	const auto& chunks = fractureChunks.begin()->second;

	// chunkIdから検索できるようにマップ化
	for (const auto& entry : chunks) {
		chunksById_[entry.info.chunkId] = &entry;
	}

	// 全チャンクの重心平均を、ひび押し出しの基準点として保持
	Vector3 sum = { 0.0f, 0.0f, 0.0f };
	for (const auto& entry : chunks) {
		sum += entry.info.centroid;
	}
	modelCenter_ = sum / static_cast<float>(chunks.size());

	// 全チャンクが無傷の状態で、1つの静的なメッシュとして描画する
	PackedGeometryBuffer* buffer = model_->GetFractureBuffers().at(groupName_).get();
	staticMesh_.Initialize(buffer);
	RebuildStaticState();
}

void FractureDamageController::Update(float deltaTime) {
	if (isReassembling_) {
		// 再構築中は通常の落下・ひび揺れシミュレーションは行わない
		UpdateReassembly(deltaTime);
		return;
	}

	SimulateCrackPhysics();

	// マクロ破片・Intact()はレイトレ提出時にworldTransformを掛けるので、ここでは何もしない
	for (auto& batch : breakState_.MacroDebrisBatches()) {
		SimulateFallingDebris(*batch, deltaTime);
	}
	// マイクロ破片はラスタライズ描画のため、生成元オブジェクトのワールド行列を毎フレーム反映する
	for (auto& batch : breakState_.MicroDebrisBatches()) {
		batch->SetParentWorldMatrix(worldMatrix_);
		SimulateFallingDebris(*batch, deltaTime);
	}
	// 凹んだチャンクは物理を掛けないため、ワールド行列の更新だけ行う
	for (auto& [chunkId, instance] : breakState_.DentedChunks()) {
		instance->SetParentWorldMatrix(worldMatrix_);
		instance->Update();
	}
}

void FractureDamageController::BeginReassembly() {
	// 何も壊れていなければ何もしない
	if (destroyedChunkIds_.empty() && !breakState_.HasDentedChunks()
		&& !breakState_.HasMacroDebris() && !breakState_.HasMicroDebris()) {
		return;
	}

	breakState_.MicroDebrisBatches().clear();

	isReassembling_ = true;
}

void FractureDamageController::UpdateReassembly(float deltaTime) {
	bool allSettled = true;
	for (auto& batch : breakState_.MacroDebrisBatches()) {
		if (!SimulateReassemblySpring(*batch, deltaTime)) {
			allSettled = false;
		}
	}

	// まだ元の位置へ戻りきっていない破片がある間はここで終わる
	if (!allSettled) {
		return;
	}

	// 全てのマクロ破片が元の位置、回転へ収束したので、まとめて無傷状態へ復元する
	destroyedChunkIds_.clear();
	chunkDamage_.clear();
	crackActiveChunkIds_.clear();
	looseChunkIds_.clear();
	breakState_.DentedChunks().clear();
	breakState_.MacroDebrisBatches().clear();

	// 壊れた、凹んだ、個別に描画するチャンクが全て空になったので、全チャンクが1つの静的なメッシュに戻る
	RebuildStaticState();

	isReassembling_ = false;
}

bool FractureDamageController::SimulateReassemblySpring(FractureInstance& instance, float deltaTime) {
	bool allSettled = true;
	auto& transforms = instance.GetTransformDatas();

	for (auto& state : transforms) {
		// 目標からのズレは、現在の位置、回転そのもの
		Vector3 displacement = state.transform.translate;
		Vector3 accel = displacement * -kReassemblySpringStiffness_ - state.crackVelocity * kReassemblyDamping_;
		state.crackVelocity += accel * deltaTime;
		state.transform.translate += state.crackVelocity * deltaTime;

		Vector3 rotAccel = state.transform.rotate * -kReassemblyAngularSpringStiffness_ - state.crackAngularVelocity * kReassemblyAngularDamping_;
		state.crackAngularVelocity += rotAccel * deltaTime;
		state.transform.rotate += state.crackAngularVelocity * deltaTime;

		bool settled = displacement.Length() < kReassemblySettleThreshold_
			&& state.crackVelocity.Length() < kReassemblySettleThreshold_
			&& state.transform.rotate.Length() < kReassemblySettleThreshold_
			&& state.crackAngularVelocity.Length() < kReassemblySettleThreshold_;
		if (settled) {
			// 収束済みなら完全に原点へスナップして、揺れ戻りを防ぐ
			state.transform.translate = { 0.0f, 0.0f, 0.0f };
			state.transform.rotate = { 0.0f, 0.0f, 0.0f };
			state.crackVelocity = { 0.0f, 0.0f, 0.0f };
			state.crackAngularVelocity = { 0.0f, 0.0f, 0.0f };
		} else {
			allSettled = false;
		}
	}

	instance.Update();
	return allSettled;
}

void FractureDamageController::ApplyChipDamage(const Vector3& impactPos, float damageAmount, float craterRadius, int craterPlaneCount,
	const Vector3& impactDirection, float penetrationDepth) {
	if (chunksById_.empty()) {
		return;
	}
	// 再構築中はダメージを受け付けない
	if (isReassembling_) {
		return;
	}

	std::optional<uint32_t> targetChunkId = FindNearestChunk(impactPos);
	if (!targetChunkId.has_value()) {
		return;
	}
	uint32_t chunkId = targetChunkId.value();

	float& accumulated = chunkDamage_[chunkId];
	accumulated += damageAmount;

	float ratio = std::min(accumulated / kBreakThreshold_, 1.0f);

	if (accumulated >= kBreakThreshold_) {
		chunkDamage_.erase(chunkId);
		// 切り離すので、付着したまま凹んでいた表示があれば不要になる
		breakState_.RemoveDentedChunk(chunkId);
		ApplyDamage(impactPos, kBreakDetachRadius_, craterRadius, craterPlaneCount, impactDirection, penetrationDepth);
		return;
	}

	// まだ切り離すほどではないので、付着させたまま動的に断面を凹ませる
	CarveAttachedDent(chunkId, impactPos, ratio, craterRadius, craterPlaneCount, impactDirection, penetrationDepth);

	// ひびを入れるチャンクと、その強さの倍率を集める
	std::vector<std::pair<uint32_t, float>> crackTargets;
	// ひびの揺れエフェクトは、まだ凹んでおらずレイトレ側に残っているチャンクにのみ適用する
	if (!breakState_.HasDentedChunk(chunkId)) {
		crackTargets.push_back({ chunkId, 1.0f });
	}
	// 隣接チャンクにも弱めのひびを波及させる
	if (kNeighborCrackFactor_ > 0.0f) {
		auto it = chunksById_.find(chunkId);
		if (it != chunksById_.end()) {
			for (uint32_t neighborId : it->second->info.neighborChunkIds) {
				if (destroyedChunkIds_.count(neighborId)) continue;
				if (breakState_.HasDentedChunk(neighborId)) continue;
				crackTargets.push_back({ neighborId, kNeighborCrackFactor_ });
			}
		}
	}

	// ひびで動くチャンクは静的なメッシュから外し、個別に描画する。作り直しは1回にまとめる
	bool hasNewLooseChunk = false;
	for (const auto& [targetId, factor] : crackTargets) {
		hasNewLooseChunk |= looseChunkIds_.insert(targetId).second;
	}
	if (hasNewLooseChunk) {
		RebuildStaticState();
	}

	for (const auto& [targetId, factor] : crackTargets) {
		UpdateCrackVisual(targetId, ratio * factor, damageAmount * factor);
	}
}

void FractureDamageController::CarveAttachedDent(uint32_t chunkId, const Vector3& impactPos, float damageRatio,
	float craterRadius, int craterPlaneCount, const Vector3& impactDirection, float penetrationDepth) {

	auto chunkIt = chunksById_.find(chunkId);
	if (chunkIt == chunksById_.end()) {
		return;
	}

	PackedGeometryBuffer* buffer = model_->GetFractureBuffers().at(groupName_).get();
	Fragment chunkFragment = buffer->ExtractChunk(chunkId);

	// ダメージが浅いうちは小さく、蓄積するほど大きく凹ませる
	float dentRadius = (craterRadius + penetrationDepth) * std::max(damageRatio, kMinDentRatio_);

	// チャンク自身のAABBから半径の上限を求めてクランプする
	Vector3 chunkExtent = chunkIt->second->info.aabb.max - chunkIt->second->info.aabb.min;
	float chunkRadius = chunkExtent.Length() * 0.5f;
	if (chunkRadius > 1e-4f) {
		dentRadius = std::min(dentRadius, chunkRadius * kMaxDentRadiusToChunkRatio_);
	}

	Vector3 dentCenter = ComputeDentCenter(impactPos, impactDirection, dentRadius);

	Log("CarveAttachedDent chunkId=" + std::to_string(chunkId) + " ratio=" + std::to_string(damageRatio) +
		" chunkRadius=" + std::to_string(chunkRadius) + " dentRadius=" + std::to_string(dentRadius) +
		" srcTris=" + std::to_string(chunkFragment.indices.size() / 3));

	bool wasAlreadyDented = breakState_.HasDentedChunk(chunkId);

	// numSites=1を渡すことでボロノイ分割はせず、クレーターで削った1個の形状だけを得る
	FractureInstance& dentedInstance = breakState_.GetOrCreateDentedChunk(chunkId);
	dentedInstance.ApplyRuntimeCut(chunkFragment, dentCenter, dentRadius, 1, craterPlaneCount);
	// ラスタライズ描画のため、生成直後にワールド行列を反映しておく（次のUpdate()まで原点に見えてしまうのを防ぐ）
	dentedInstance.SetParentWorldMatrix(worldMatrix_);
	dentedInstance.Update();

	// 初めて凹んだチャンクは、レイトレ描画用の静的なメッシュ、個別のインスタンスから除外する
	if (!wasAlreadyDented) {
		RebuildStaticState();
	}
}

Vector3 FractureDamageController::ComputeDentCenter(const Vector3& impactPos, const Vector3& impactDirection, float dentRadius) const {
	Vector3 dentCenter = impactPos;
	float dirLength = impactDirection.Length();
	if (dirLength > 1e-4f) {
		Vector3 dir = impactDirection / dirLength;
		// 衝突方向と逆側へ球の中心を押し込み、攻撃側の表面が開いた噛み跡のような形にする
		dentCenter -= dir * (dentRadius * kDentInwardBiasRatio_);
	}
	return dentCenter;
}

void FractureDamageController::RebuildStaticState() {
	PackedGeometryBuffer* buffer = model_->GetFractureBuffers().at(groupName_).get();

	// 壊れた、凹んだチャンクは別の描画になるので、個別に描画する対象から外す
	for (auto it = looseChunkIds_.begin(); it != looseChunkIds_.end();) {
		if (destroyedChunkIds_.count(*it) || breakState_.HasDentedChunk(*it)) {
			it = looseChunkIds_.erase(it);
		} else {
			++it;
		}
	}

	// 作り直してもひびの揺れが途切れないよう、個別に描画していたチャンクの状態を退避しておく
	std::unordered_map<uint32_t, FractureChunkState> previousStates;
	auto& previousTransforms = breakState_.Intact().GetTransformDatas();
	for (const auto& [id, index] : chunkIndexInIntact_) {
		if (index < previousTransforms.size()) {
			previousStates[id] = previousTransforms[index];
		}
	}

	std::vector<uint32_t> looseIds(looseChunkIds_.begin(), looseChunkIds_.end());
	std::sort(looseIds.begin(), looseIds.end());

	if (!looseIds.empty()) {
		breakState_.Intact().Initialize(looseIds, *buffer);
		RebuildIntactIndexMap(looseIds);

		auto& transforms = breakState_.Intact().GetTransformDatas();
		for (uint32_t i = 0; i < looseIds.size(); ++i) {
			auto it = previousStates.find(looseIds[i]);
			if (it != previousStates.end()) {
				transforms[i] = it->second;
			}
		}
		breakState_.Intact().Update();
	} else {
		breakState_.Intact().Clear();
		chunkIndexInIntact_.clear();
	}

	// 残りの静的なチャンクを、内部の切断面を除いた1つのメッシュにまとめる
	staticMesh_.Rebuild([this](uint32_t chunkId) { return IsStaticChunk(chunkId); });
}

bool FractureDamageController::IsStaticChunk(uint32_t chunkId) const {
	if (chunksById_.find(chunkId) == chunksById_.end()) { return false; }
	if (destroyedChunkIds_.count(chunkId)) { return false; }
	if (breakState_.HasDentedChunk(chunkId)) { return false; }
	if (looseChunkIds_.count(chunkId)) { return false; }
	return true;
}

void FractureDamageController::ApplyDamage(const Vector3& impactPos, float damageRadius, float craterRadius, int craterPlaneCount,
	const Vector3& impactDirection, float penetrationDepth) {
	// 破壊データがなければ飛ばす
	if (chunksById_.empty()) {
		return;
	}

	// まだ壊れていない一番近いチャンクをシードとして特定
	std::optional<uint32_t> seedChunkId = FindNearestChunk(impactPos);
	// 壊せるチャンクが残っていないければ飛ばす
	if (!seedChunkId.has_value()) {
		return;
	}

	// フラッドフィルで切り離すチャンク範囲を選ぶ
	std::vector<uint32_t> detachedIds = SelectDetachedChunks(seedChunkId.value(), damageRadius, impactPos);
	if (detachedIds.empty()) {
		return;
	}

	PackedGeometryBuffer* buffer = model_->GetFractureBuffers().at(groupName_).get();

	// シード以外は事前分割のまま切り離す
	std::vector<uint32_t> macroIds;
	macroIds.reserve(detachedIds.size());
	for (uint32_t id : detachedIds) {
		if (id != seedChunkId.value()) {
			macroIds.push_back(id);
		}
	}
	if (!macroIds.empty()) {
		// 破壊イベントごとに新しいバッチとして追加する
		FractureInstance& macroBatch = breakState_.AddMacroDebrisBatch();
		macroBatch.Initialize(macroIds, *buffer);

		// 各破片に、衝撃点からの距離に応じた爆発の初速を与える
		ApplyExplosionImpulse(macroBatch, macroIds, impactPos, damageRadius);

		breakState_.TrimMacroDebrisBatches(kMaxDebrisBatchesPerType_);
	}

	// チャンクを取得
	Fragment seedFragment = buffer->ExtractChunk(seedChunkId.value());
	Log("seed tris=" + std::to_string(seedFragment.indices.size() / 3));

	float dentRadius = craterRadius + penetrationDepth;
	Vector3 dentCenter = ComputeDentCenter(impactPos, impactDirection, dentRadius);

	// シードチャンクだけランタイムカット。破壊イベントごとに新しいバッチとして追加する
	FractureInstance& microBatch = breakState_.AddMicroDebrisBatch();
	microBatch.ApplyRuntimeCut(seedFragment, dentCenter, dentRadius, 8, craterPlaneCount);
	// ラスタライズ描画のため、生成直後にワールド行列を反映しておく（次のUpdate()まで原点に見えてしまうのを防ぐ）
	microBatch.SetParentWorldMatrix(worldMatrix_);
	microBatch.Update();

	// シード由来の破片に爆発の初速を与える
	ApplyExplosionImpulseUniform(microBatch, impactPos, 8.0f);

	Log("microDebris numInstance = " + std::to_string(microBatch.GetNumInstance()));

	breakState_.TrimMicroDebrisBatches(kMaxDebrisBatchesPerType_);

	// 切り離したチャンクを記録する
	for (uint32_t id : detachedIds) {
		destroyedChunkIds_.insert(id);
		breakState_.RemoveDentedChunk(id);
	}

	// 破壊されたチャンクの蓄積情報は不要になるので削除
	for (uint32_t id : detachedIds) {
		chunkDamage_.erase(id);
		crackActiveChunkIds_.erase(id);
	}

	// 直接の破壊でアンカーへの経路が絶たれ、支えを失ったチャンクがあれば併せて崩落させる
	CollapseUnsupportedChunks();

	// 直接破壊分・崩落分の両方を反映して、静的なメッシュを1回だけ作り直す
	RebuildStaticState();
}

void FractureDamageController::CollapseUnsupportedChunks() {
	// アンカーかつ未破壊のチャンクを起点に、隣接グラフ上で到達できる範囲を求める
	std::unordered_set<uint32_t> reachable;
	std::queue<uint32_t> queue;
	bool hasAnyAnchor = false;

	for (const auto& [id, entry] : chunksById_) {
		if (destroyedChunkIds_.count(id)) { continue; }
		if (!entry->info.isAnchored) { continue; }
		hasAnyAnchor = true;
		if (reachable.insert(id).second) {
			queue.push(id);
		}
	}

	// アンカーが1つもなければ、崩落判定自体をスキップする
	if (!hasAnyAnchor) {
		return;
	}

	while (!queue.empty()) {
		uint32_t id = queue.front();
		queue.pop();

		auto it = chunksById_.find(id);
		if (it == chunksById_.end()) {
			continue;
		}

		for (uint32_t neighborId : it->second->info.neighborChunkIds) {
			if (destroyedChunkIds_.count(neighborId)) { continue; }
			if (chunksById_.find(neighborId) == chunksById_.end()) { continue; }
			if (reachable.insert(neighborId).second) {
				queue.push(neighborId);
			}
		}
	}

	// アンカーから到達できない、まだ破壊されていないチャンク
	std::vector<uint32_t> unsupportedIds;
	for (const auto& [id, entry] : chunksById_) {
		if (destroyedChunkIds_.count(id)) { continue; }
		if (reachable.count(id)) { continue; }
		unsupportedIds.push_back(id);
	}
	if (unsupportedIds.empty()) {
		return;
	}

	Log("CollapseUnsupportedChunks count=" + std::to_string(unsupportedIds.size()));

	PackedGeometryBuffer* buffer = model_->GetFractureBuffers().at(groupName_).get();

	// 崩落するチャンクも、事前分割のまま切り離すマクロ破片と同じ扱いで新しいバッチにする
	FractureInstance& collapseBatch = breakState_.AddMacroDebrisBatch();
	collapseBatch.Initialize(unsupportedIds, *buffer);
	ApplyCollapseImpulse(collapseBatch);
	breakState_.TrimMacroDebrisBatches(kMaxDebrisBatchesPerType_);

	for (uint32_t id : unsupportedIds) {
		destroyedChunkIds_.insert(id);
		breakState_.RemoveDentedChunk(id);
		chunkDamage_.erase(id);
		crackActiveChunkIds_.erase(id);
	}
}

std::vector<uint32_t> FractureDamageController::SelectDetachedChunks(uint32_t seedChunkId, float damageRadius, const Vector3& impactPos) const {
	std::queue<uint32_t> queue;
	std::unordered_set<uint32_t> visited;
	std::vector<uint32_t> result;

	queue.push(seedChunkId);
	visited.insert(seedChunkId);

	while (!queue.empty()) {
		uint32_t id = queue.front();
		queue.pop();

		auto it = chunksById_.find(id);
		if (it == chunksById_.end()) {
			continue;
		}
		const FractureChunkInfo& info = it->second->info;

		// アンカーチャンクは境界として残し、切り離さない
		if (info.isAnchored) {
			continue;
		}

		result.push_back(id);

		for (uint32_t neighborId : info.neighborChunkIds) {
			if (visited.count(neighborId)) {
				continue;
			}
			// 既に壊れているチャンクは選ばない
			if (destroyedChunkIds_.count(neighborId)) {
				continue;
			}
			auto neighborIt = chunksById_.find(neighborId);
			if (neighborIt == chunksById_.end()) {
				continue;
			}
			// ダメージ半径の外
			float dist = Vector3::Distance(neighborIt->second->info.centroid, impactPos);
			if (dist > damageRadius) {
				continue;
			}
			visited.insert(neighborId);
			queue.push(neighborId);
		}
	}

	return result;
}

std::optional<uint32_t> FractureDamageController::FindNearestChunk(const Vector3& impactPos) const {
	std::optional<uint32_t> nearestId;
	float nearestDistSq = FLT_MAX;

	for (const auto& [chunkId, entry] : chunksById_) {
		// 既に壊れているチャンクは飛ばす
		if (destroyedChunkIds_.find(chunkId) != destroyedChunkIds_.end()) {
			continue;
		}
		Vector3 diff = entry->info.centroid - impactPos;
		float distSq = diff.Dot(diff);
		if (distSq < nearestDistSq) {
			nearestDistSq = distSq;
			nearestId = chunkId;
		}
	}
	return nearestId;
}

void FractureDamageController::SimulateFallingDebris(FractureInstance& instance, float deltaTime) {
	auto& transforms = instance.GetTransformDatas();
	for (auto& state : transforms) {
		state.velocity.y -= 9.8f * deltaTime;
		state.transform.translate += state.velocity * deltaTime;
		state.transform.rotate.x += 1.0f * deltaTime;
		state.transform.rotate.y += 0.6f * deltaTime;
	}
	instance.Update();
}

void FractureDamageController::ApplyExplosionImpulse(FractureInstance& instance,
	const std::vector<uint32_t>& chunkIds, const Vector3& impactPos, float damageRadius) {

	auto& transforms = instance.GetTransformDatas();
	for (size_t i = 0; i < chunkIds.size(); ++i) {
		const auto& entry = chunksById_.at(chunkIds[i]);
		Vector3 dir = entry->info.centroid - impactPos;
		float dist = dir.Length();
		if (dist < 1e-4f) {
			// 中心とほぼ同じ位置なら上方向にフォールバック
			dir = Vector3(0.0f, 1.0f, 0.0f);
			dist = 1e-4f;
		} else {
			// 正規化
			dir = dir / dist;
		}

		// 中心に近いほど強く、遠いほど弱い
		float falloff = std::max(0.0f, 1.0f - dist / damageRadius);
		// 最低速度3、中心付近で最大12程度
		float speed = 3.0f + falloff * 9.0f;

		transforms[i].velocity = dir * speed;
		transforms[i].velocity += RandomGenerator::GetVector3(-0.5f, 0.5f);
	}
}

void FractureDamageController::ApplyExplosionImpulseUniform(FractureInstance& instance,
	const Vector3& impactPos, float strength) {

	// ランタイムカット破片には個別の重心情報がないので、破片ごとにランダム方向へ均等に飛ばす
	auto& transforms = instance.GetTransformDatas();
	for (auto& state : transforms) {
		Vector3 randomDir = RandomGenerator::GetVector3(-1.0f, 1.0f);
		randomDir.Normalize();
		state.velocity = randomDir * strength + Vector3(0.0f, strength * 0.3f, 0.0f);
	}
}

void FractureDamageController::ApplyCollapseImpulse(FractureInstance& instance) {
	auto& transforms = instance.GetTransformDatas();
	for (auto& state : transforms) {
		state.velocity = RandomGenerator::GetVector3(-0.3f, 0.3f);
		state.velocity.y = std::min(state.velocity.y, 0.0f); // 上向きには飛ばさない
	}
}

void FractureDamageController::UpdateCrackVisual(uint32_t chunkId, float ratio, float damageDelta) {
	auto indexIt = chunkIndexInIntact_.find(chunkId);
	auto chunkIt = chunksById_.find(chunkId);
	if (indexIt == chunkIndexInIntact_.end() || chunkIt == chunksById_.end()) {
		return;
	}

	Vector3 dir = chunkIt->second->info.centroid - modelCenter_;
	float len = dir.Length();
	if (len < 1e-4f) {
		dir = Vector3(0.0f, 1.0f, 0.0f);
	} else {
		dir = dir / len;
	}

	auto& transforms = breakState_.Intact().GetTransformDatas();
	FractureChunkState& state = transforms[indexIt->second];

	// ダメージが蓄積するほど外側に押し出された位置になる
	state.crackRestOffset = dir * (ratio * kMaxCrackOffset_);

	// 衝撃の瞬間速度インパルス
	float impulse = kCrackImpulseStrength_ * damageDelta;
	state.crackVelocity += dir * impulse;
	state.crackAngularVelocity += RandomGenerator::GetVector3(-1.0f, 1.0f) * impulse * kMaxCrackRotate_;

	// このチャンクは以後、毎フレームのばねシミュレーション対象にする
	crackActiveChunkIds_.insert(chunkId);

	breakState_.Intact().Update();
}

void FractureDamageController::RebuildIntactIndexMap(const std::vector<uint32_t>& ids) {
	chunkIndexInIntact_.clear();
	for (uint32_t i = 0; i < ids.size(); ++i) {
		chunkIndexInIntact_[ids[i]] = i;
	}
}

void FractureDamageController::SimulateCrackPhysics() {
	// 揺れているチャンクがなければ飛ばず
	if (crackActiveChunkIds_.empty()) {
		return;
	}

	auto& transforms = breakState_.Intact().GetTransformDatas();
	// 収束したとみなす値
	constexpr float kSettleThreshold = 0.0005f;

	std::vector<uint32_t> settledIds;

	for (uint32_t chunkId : crackActiveChunkIds_) {
		auto indexIt = chunkIndexInIntact_.find(chunkId);
		// 壊れて消えた等、対象にしない
		if (indexIt == chunkIndexInIntact_.end()) {
			settledIds.push_back(chunkId);
			continue;
		}

		FractureChunkState& state = transforms[indexIt->second];

		// 位置のばね
		Vector3 displacement = state.transform.translate - state.crackRestOffset;
		Vector3 accel = displacement * -kCrackSpringStiffness_ - state.crackVelocity * kCrackDamping_;
		state.crackVelocity += accel * FpsCounter::gameDeltaTime;
		state.transform.translate += state.crackVelocity * FpsCounter::gameDeltaTime;

		// 回転のばね
		Vector3 rotAccel = state.transform.rotate * -kCrackAngularSpringStiffness_ - state.crackAngularVelocity * kCrackAngularDamping_;
		state.crackAngularVelocity += rotAccel * FpsCounter::gameDeltaTime;
		state.transform.rotate += state.crackAngularVelocity * FpsCounter::gameDeltaTime;

		// 十分収まったらアクティブリストから外す
		bool settled = displacement.Length() < kSettleThreshold
			&& state.crackVelocity.Length() < kSettleThreshold
			&& state.transform.rotate.Length() < kSettleThreshold
			&& state.crackAngularVelocity.Length() < kSettleThreshold;
		if (settled) {
			settledIds.push_back(chunkId);
		}
	}

	bool hasReturnedChunk = false;
	for (uint32_t id : settledIds) {
		crackActiveChunkIds_.erase(id);

		// 揺れが収まり、元の位置からのずれも残っていなければ静的なメッシュに戻す
		auto indexIt = chunkIndexInIntact_.find(id);
		if (indexIt == chunkIndexInIntact_.end()) {
			continue;
		}
		if (transforms[indexIt->second].crackRestOffset.Length() < kSettleThreshold) {
			looseChunkIds_.erase(id);
			hasReturnedChunk = true;
		}
	}

	breakState_.Intact().Update();

	if (hasReturnedChunk) {
		RebuildStaticState();
	}
}

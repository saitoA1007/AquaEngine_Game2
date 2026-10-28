#include "pch.h"
#include "EffectObject.h"
#include <algorithm>
#include "ModelManager.h"
#include "FPSCounter.h"
#include "LogManager.h"

using namespace GameEngine;

EffectObject::EffectObject(const EffectAsset& asset, TextureManager* textureManager, ModelManager* modelManager) {
	name_ = asset.name;
	duration_ = asset.duration;
	isLoop_ = asset.isLoop;

	for (size_t i = 0; i < asset.tracks.size(); ++i) {
		const EffectTrackData& track = asset.tracks[i];
		if (track.particleName.empty()) {
			continue;
		}
		Model* model = modelManager->GetNameByModel(track.modelName);
		if (model == nullptr) {
			Log("EffectObject: " + name_ + " / " + track.name + " : model \"" + track.modelName + "\" not found");
			continue;
		}
		modules_.push_back(std::make_unique<EffectModule>(track, textureManager, model));
		trackIndices_.push_back(i);
	}
}

void EffectObject::ApplyTiming(const EffectAsset& asset) {
	duration_ = asset.duration;
	isLoop_ = asset.isLoop;

	for (size_t i = 0; i < modules_.size(); ++i) {
		if (trackIndices_[i] < asset.tracks.size()) {
			modules_[i]->SetTrackData(asset.tracks[trackIndices_[i]]);
		}
	}
}

void EffectObject::Update() {
	Update(FpsCounter::deltaTime);
}

void EffectObject::Seek(float targetTime, float step) {
	targetTime = std::clamp(targetTime, 0.0f, duration_);

	// 戻る場合は最初から再生し直す
	if (!isPlaying_ || targetTime < time_) {
		Play(position_);
	}

	// 進める時間は先に決めておく
	float remaining = targetTime - time_;
	while (remaining > 0.0f && isPlaying_) {
		float dt = (std::min)(step, remaining);
		Update(dt);
		remaining -= dt;
	}
}

void EffectObject::Update(float deltaTime) {
	if (!isPlaying_) { return; }

	if (!isEmitFinished_) {
		float prevTime = time_;
		time_ += deltaTime;

		if (time_ >= duration_) {
			if (isLoop_ && duration_ > 0.0f) {
				// 終端までの区間を処理してから先頭に戻る
				UpdateEmission(prevTime, duration_);
				time_ -= duration_;
				UpdateEmission(0.0f, time_);
			} else {
				// 最後の区間を処理して発生を終了する
				UpdateEmission(prevTime, duration_);
				for (auto& module : modules_) {
					module->StopEmission();
				}
				time_ = duration_;
				isEmitFinished_ = true;
			}
		} else {
			UpdateEmission(prevTime, time_);
		}
	}

	// パーティクルの更新
	for (auto& module : modules_) {
		module->Update(deltaTime);
	}

	// 発生が終わり、パーティクルが全て消えたら再生終了
	if (isEmitFinished_ && !HasAliveParticles()) {
		isPlaying_ = false;
	}
}

void EffectObject::Draw() {
	if (!isPlaying_) { return; }

	for (auto& module : modules_) {
		module->Draw();
	}
}

void EffectObject::Play(const Vector3& pos) {
	position_ = pos;
	time_ = 0.0f;
	isPlaying_ = true;
	isEmitFinished_ = false;

	for (auto& module : modules_) {
		module->Reset();
	}
}

void EffectObject::Stop() {
	if (!isPlaying_) { return; }

	for (auto& module : modules_) {
		module->StopEmission();
	}
	isEmitFinished_ = true;
}

void EffectObject::StopImmediate() {
	for (auto& module : modules_) {
		module->Reset();
	}
	isPlaying_ = false;
	isEmitFinished_ = true;
}

void EffectObject::UpdateEmission(float prevTime, float time) {
	for (auto& module : modules_) {
		module->UpdateEmission(prevTime, time, position_);
	}
}

bool EffectObject::HasAliveParticles() const {
	for (const auto& module : modules_) {
		if (module->HasAliveParticles()) {
			return true;
		}
	}
	return false;
}

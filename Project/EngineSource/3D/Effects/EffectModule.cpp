#include "pch.h"
#include "EffectModule.h"
using namespace GameEngine;

EffectModule::EffectModule(const EffectTrackData& data, TextureManager* textureManager, Model* model) {
	data_ = data;
	particle_ = std::make_unique<ParticleBehavior>(data_.particleName, data_.maxNum, textureManager, model);
	particle_->Initialize();
	ApplyEmitMode();
}

void EffectModule::UpdateEmission(float prevTime, float time, const Vector3& basePos) {
	const Vector3 emitPos = basePos + data_.offset;
	particle_->SetEmitterPos(emitPos);
	// 引き寄せ・らせんの中心も発生位置に合わせる
	particle_->SetAttractionTarget(emitPos);

	if (data_.emitMode == EffectEmitMode::kBurst) {
		// 開始時間をこのフレームで通過したら1回だけ発生させる
		if (prevTime <= data_.startTime && data_.startTime < time) {
			particle_->Emit(emitPos);
		}
	} else {
		// 開始時間から終了時間の間だけ連続で発生させる
		particle_->SetEmitting(data_.startTime <= time && time < data_.GetEndTime());
	}
}

void EffectModule::Update(float deltaTime) {
	particle_->Update(deltaTime);
}

void EffectModule::Draw() {
	particle_->Draw();
}

void EffectModule::StopEmission() {
	particle_->SetEmitting(false);
}

void EffectModule::Reset() {
	particle_->Clear();
	ApplyEmitMode();
}

void EffectModule::SetTrackData(const EffectTrackData& data) {
	data_.name = data.name;
	data_.emitMode = data.emitMode;
	data_.startTime = data.startTime;
	data_.duration = data.duration;
	data_.offset = data.offset;
	ApplyEmitMode();
}

void EffectModule::ApplyEmitMode() {
	if (data_.emitMode == EffectEmitMode::kBurst) {
		// Emitで発生させるためループを無効にする
		particle_->SetIsLoop(false);
	} else {
		// ループで発生させ、SetEmittingで制御する
		particle_->SetIsLoop(true);
		particle_->SetEmitting(false);
	}
}

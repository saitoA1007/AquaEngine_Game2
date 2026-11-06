#include "pch.h"
#include "IceMistEffect.h"
using namespace GameEngine;

IceMistEffect::IceMistEffect(Model* planeModel, TextureManager* textureManager) {
    particle_ = std::make_unique<ParticleBehavior>(kParticleName, kMaxParticles, textureManager, planeModel);
    particle_->Initialize();

    // 氷霧のたびにEmitで1回だけ発生させるため、ループは無効にする
    particle_->SetIsLoop(false);
}

void IceMistEffect::Update() {
    particle_->Update();
}

void IceMistEffect::Draw() {
    particle_->Draw();
}

void IceMistEffect::Play(const Vector3& center, float radius) {
    // 氷エリアの円の範囲から発生させる
    Vector3 emitPosition = { center.x, kEmitHeight, center.z };
    particle_->SetShapeEmit(radius);
    particle_->SetEmitterPos(emitPosition);
    particle_->Emit(emitPosition);
}

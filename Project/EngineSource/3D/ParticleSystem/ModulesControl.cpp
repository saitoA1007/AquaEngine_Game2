#include "pch.h"
#include "ModulesControl.h"
#include "ParticleEmitModules.h"
#include "ParticleUpdateModules.h"
using namespace GameEngine;

ModulesControl::ModulesControl(DebugParameter* param) {
    param_ = param;

    std::string mainGroup = "Emitter";

    // モジュールを登録
    RegisterModule<TextureModule>(mainGroup, "TextureEmit");
    RegisterModule<VelocityEmitModule>(mainGroup, "VelocityEmit");
    RegisterModule<DirectionEmitModule>(mainGroup, "DirectionEmit");
    RegisterModule<RotateEmitModule>(mainGroup, "RotateEmit");
    RegisterModule<ScaleEmitModule>(mainGroup, "ScaleEmit");
    RegisterModule<ShapeEmitModule>(mainGroup, "ShapeEmit");
    RegisterModule<ColorEmitModule>(mainGroup, "ColorEmit");
    RegisterModule<RotateVelocityEmitModule>(mainGroup, "RotateVelocityEmit");
    RegisterModule<LifeTimeEmitModule>(mainGroup, "LifeTimeEmit");

    mainGroup = "Particle";
    RegisterModule<VelocityOverLifeTimeModule>(mainGroup, "VelocityOverLifeTime");
    RegisterModule<SizeOverLifeTimeModule>(mainGroup, "SizeOverLifeTime");
    RegisterModule<AlphaOverLifeTimeModule>(mainGroup, "AlphaOverLifeTime");
    RegisterModule<ColorOverLifeTimeModule>(mainGroup, "ColorOverLifeTime");
    RegisterModule<AttractionModule>(mainGroup, "Attraction");
    RegisterModule<VortexModule>(mainGroup, "Vortex");
    RegisterModule<RotationByVelocityModule>(mainGroup, "RotationByVelocity");
    RegisterModule<TrailModule>(mainGroup, "Trail");
    RegisterModule<GravityFieldModule>(mainGroup, "GravityField");
}

void ModulesControl::Update() {

    // モジュール管理
    for (auto& [name, moduleData] : enableModules_) {

        if (moduleData.isActive) {
            if (moduleData.isCreated) { continue; }

            // モジュールを作成する
            modules_[name] = moduleData.maker();
            modules_[name]->SetGroupName(moduleData.mainGroupName,name);
            modules_[name]->Register(param_);
            moduleData.isCreated = true;
        } else {
            if (!moduleData.isCreated) { continue; }

            // モジュールの登録を解除する
            modules_[name]->Remove(param_);
            modules_.erase(name);
            moduleData.isCreated = false;
        }
    }
}

void ModulesControl::ParticleCreate(ParticleData& particleData) {
    // パーティクルを作成
    for (auto& [name, module] : modules_) {
        module->Create(particleData);
    }
}

void ModulesControl::ParticleUpdate(ParticleData& particleData, float time) {
    // パーティクルを更新
    for (auto& [name, module] : modules_) {
        module->Update(particleData, time);
    }
}
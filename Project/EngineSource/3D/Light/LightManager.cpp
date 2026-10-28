#include "pch.h"
#include"LightManager.h"
#include"EngineSource/Common/CreateBufferResource.h"
#include"EngineSource/Math/MyMath.h"
using namespace GameEngine;

LightManager::~LightManager() {

}

void LightManager::Initialize(const bool& isDirectionalActive, const bool& isPointActive, const bool& isSpotActive) {
    // 平行光源
    directionalLight_ = std::make_unique<DirectionalLight>();
    directionalLight_->Initialize({ 1,1,1,1 }, { 0,-1,0 }, 1.0f);
    directionalLight_->SetActive(isDirectionalActive);
    // 点光源
    pointLight_ = std::make_unique<PointLight>();
    pointLight_->Initialize({ 1,1,1,1 }, { 0,0,0 }, 1.0f);
    pointLight_->SetLightActive(isPointActive);
    // スポットライト
    spotLight_ = std::make_unique<SpotLight>();
    spotLight_->Initialize({ 1,1,1,1 }, { 0,0,0 }, 1.0f);
    spotLight_->SetLightActive(isSpotActive);

    // 定数バッファの作成
    constBuffer_.Create();
    lightGroupData_ = constBuffer_.GetData();
    // デフォルト値を設定
    lightGroupData_->directionalLightData_ = directionalLight_->GetDirectionalLightData();
    lightGroupData_->pointLightData_ = pointLight_->GetPointLightData();
    lightGroupData_->spotLightData_ = spotLight_->GetSpotLightData();
}

void LightManager::Update() {
    // 更新
    if (lightGroupData_->directionalLightData_.active) {
        lightGroupData_->directionalLightData_ = directionalLight_->GetDirectionalLightData();
    }
    if (lightGroupData_->pointLightData_.active) {
        lightGroupData_->pointLightData_ = pointLight_->GetPointLightData();
    }
    if (lightGroupData_->spotLightData_.active) {
        lightGroupData_->spotLightData_ = spotLight_->GetSpotLightData();
    }
}

void LightManager::SetDirectionalData(const DirectionalLight::DirectionalLightData& directionalData) {
    directionalLight_->SetDirectionalLightData(directionalData);
}

void LightManager::Setshadow(const Vector3& targetCenter, float shadowRange) {
    directionalLight_->CreateDirectionalShadowMatrix(targetCenter, shadowRange);
}

void  LightManager::SetDirectionalDirction(const Vector3& lightdir) {
    directionalLight_->SetLightDir(lightdir);
}

void LightManager::SetDirectionalIntensity(const float& intensity) {
    directionalLight_->SetLightIntensity(intensity);
}

void LightManager::SetDirectionalColor(Vector4 color) {
    directionalLight_->SetLightColor(color);
}

void LightManager::SetDirectionalLightActive(const bool& active) {
    directionalLight_->SetActive(active);
}

void LightManager::SetPointData(const PointLight::PointLightData& pointData) {
    pointLight_->SetPointLightData(pointData);
}

void LightManager::SetPointLightPosition(const Vector3& position) {
    pointLight_->SetLightPosition(position);
}

void LightManager::SetPointLightActive(const bool& active) {
    pointLight_->SetLightActive(active);
}

void LightManager::SetSpotData(const SpotLight::SpotLightData& spotData) {
    spotLight_->SetSpotLightData(spotData);
}

void LightManager::SetSpotLightPosition(const Vector3& position) {
    spotLight_->SetLightPosition(position);
}

void LightManager::SetSpotLightActive(const bool& active) {
    spotLight_->SetLightActive(active);
}
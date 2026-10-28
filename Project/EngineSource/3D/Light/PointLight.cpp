#include "pch.h"
#include"PointLight.h"
#include"EngineSource/Math/MyMath.h"
using namespace GameEngine;

void PointLight::Initialize(const Vector4& color, const Vector3& position, const float& intensity) {
	// デフォルト値を設定
	pointLightData_.color = color;// 色
	pointLightData_.position = position;// 位置
	pointLightData_.intensity = intensity;// 輝度
	pointLightData_.active = false;
	pointLightData_.radius = 4.0f;
	pointLightData_.decay = 2.0f;
}

void PointLight::SetLightPosition(const Vector3& position) {
	pointLightData_.position = position;
}

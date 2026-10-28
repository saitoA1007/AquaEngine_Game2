#include "pch.h"
#include"SpotLight.h"
#include"EngineSource/Math/MyMath.h"
using namespace GameEngine;

void SpotLight::Initialize(const Vector4& color, const Vector3& position, const float& intensity) {
	// デフォルト値を設定
	spotLightData_.color = color;// 色
	spotLightData_.position = position;// 位置
	spotLightData_.intensity = intensity;// 輝度
	spotLightData_.direction = { 0.0f,-1.0f,0.0f }; // ライトの方向
	spotLightData_.distance = 4.0f; // ライトの最大距離
	spotLightData_.decay = 2.0f; // 減衰率
	spotLightData_.cosAngle = 0.0f; // スポットライトの余弦
	spotLightData_.cosFalloffStart = 0.0f; // 光の角度
	spotLightData_.active = false; // ライトの使用
}

void SpotLight::SetLightPosition(const Vector3& position) {
	spotLightData_.position = position;
}


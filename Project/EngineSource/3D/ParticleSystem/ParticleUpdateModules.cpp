#include "pch.h"
#include "ParticleUpdateModules.h"
#include "MyMath.h"
#include <algorithm>
using namespace GameEngine;

//==================================================
// 速度変化モジュール
//==================================================

void VelocityOverLifeTimeModule::Update(ParticleData& particleData, [[maybe_unused]] float time) {
	// 速度を補間
	particleData.velocity = Lerp(particleData.startSpeed, endVelocity_, particleData.currentTime, easeCurve_);
}

//==================================================
// サイズ変化モジュール
//==================================================

void SizeOverLifeTimeModule::Update(ParticleData& particleData, [[maybe_unused]] float time) {

	if (separateAxes_) {
		particleData.transform.scale = Lerp(particleData.startSize, separateAxesEndSize_, particleData.currentTime, easeCurve_);
	} else {
		particleData.transform.scale = Lerp(particleData.startSize, Vector3(endSize_, endSize_, endSize_), particleData.currentTime, easeCurve_);
	}
}

//==================================================
// 透明度変化モジュール
//==================================================

void AlphaOverLifeTimeModule::Update(ParticleData& particleData, [[maybe_unused]] float time) {
	particleData.color.w = Lerp(particleData.startColor.w, endAlpha_, particleData.currentTime, easeCurve_);
}

//==================================================
// 色変化モジュール
//==================================================

void ColorOverLifeTimeModule::Create(ParticleData& particleData) {
	startHSV_ = Math::RGBtoHSV({ particleData.startColor.x,particleData.startColor.y,particleData.startColor.z });
}

void ColorOverLifeTimeModule::Update(ParticleData& particleData, [[maybe_unused]] float time) {
	//Vector3 startHsv = Math::RGBtoHSV({ particleData.startColor.x,particleData.startColor.y,particleData.startColor.z });
	Vector3 endHsv = Math::RGBtoHSV({ endRGB_.x,endRGB_.y,endRGB_.z });
	
	Vector3 hsv = Lerp(startHSV_, endHsv, particleData.currentTime, easeCurve_);

	// RGBに変換して反映する
	Vector3 rgb = Math::HSVtoRGB(hsv.x, std::clamp(hsv.y, 0.0f, 1.0f), std::clamp(hsv.z, 0.0f, 1.0f));
	particleData.color.x = rgb.x;
	particleData.color.y = rgb.y;
	particleData.color.z = rgb.z;
}

//==================================================
// 引力モジュール
//==================================================

void AttractionModule::Update(ParticleData& particleData, float time) {
	// 目標位置へのベクトルを計算
	Vector3 toTarget = targetPos_ - particleData.transform.translate;
	float distanceSquared = toTarget.LengthSquared();

	if (distanceSquared > 0.0001f) {
		Vector3 direction = toTarget.Normalize();

		// 目標に向かう加速度を現在の速度に加算
		particleData.velocity += direction * strength_ * time;
	}

	// 速度の減衰
	particleData.velocity = particleData.velocity * (1.0f - damping_ * time);
}

//==================================================
// らせんモジュール
//==================================================

void VortexModule::Update(ParticleData& particleData, float time) {
	// 中心点からパーティクルへのベクトル
	Vector3 offset = particleData.transform.translate - centerPos_;
	offset.y = 0.0f;

	// Y軸まわりの回転を計算するため、XZ平面上での距離を測る
	float distanceXZ = offset.Length();

	if (distanceXZ > 0.0001f) {
		// 回転方向のベクトルを計算
		Vector3 tangent;
		tangent.x = -offset.z / distanceXZ;
		tangent.y = 0.0f;
		tangent.z = offset.x / distanceXZ;

		// 中心に向かうベクトルを計算
		Vector3 inward;
		inward.x = -offset.x / distanceXZ;
		inward.y = 0.0f;
		inward.z = -offset.z / distanceXZ;

		// 目標速度
		Vector3 targetVelocity = { 0.0f, 0.0f, 0.0f };

		// 回転速度を適用
		targetVelocity += tangent * rotationSpeed_;

		// 吸い込み
		targetVelocity += inward * attractionSpeed_;

		// 軸方向の速度を適用
		targetVelocity.y = axisSpeed_;

		// 現在の速度から目標の渦速度へ徐々に近づける
		particleData.velocity += targetVelocity * time;
	}
}

//==================================================
// 重力場モジュール
//==================================================

void GravityFieldModule::Update(ParticleData& particleData, float time) {
	// 方向が設定されていない時は何もしない
	if (direction_.LengthSquared() <= 0.0001f) {
		return;
	}

	// 強さがマイナスなら逆方向に働かせる
	Vector3 direction = direction_.Normalize();
	if (strength_ < 0.0f) {
		direction = direction * -1.0f;
	}

	// 重力方向に加速させる
	particleData.velocity += direction * std::fabs(strength_) * time;

	// 重力方向の速度を上限で抑える
	if (maxSpeed_ > 0.0f) {
		float speed = particleData.velocity.Dot(direction);
		if (speed > maxSpeed_) {
			particleData.velocity -= direction * (speed - maxSpeed_);
		}
	}
}

//==================================================
// 速度方向に回転させるモジュール
//==================================================

void RotationByVelocityModule::Update(ParticleData& particleData, [[maybe_unused]] float time) {
	Vector3 vel = particleData.velocity;
	float lenSq = vel.LengthSquared();

	// 速度がほぼ0のときは、直前の向きを維持す
	if (lenSq > 0.0001f) {
		Vector3 euler = Math::DirectionToEuler(vel);

		// パーティクルの回転に適用
		particleData.transform.rotate.x = euler.x;
		particleData.transform.rotate.y = euler.y;
	}
}

//==================================================
// トレイルモジュール
//==================================================

void TrailModule::Create(ParticleData& particleData) {
	// 軌跡をリセット
	particleData.trailHead = 0;
	particleData.trailCount = 0;
	particleData.trailTimer = 0.0f;
}

void TrailModule::Update(ParticleData& particleData, float time) {
	particleData.trailTimer += time;
	if (particleData.trailTimer < recordInterval_) {
		return;
	}
	particleData.trailTimer = 0.0f;

	// 現在の位置を記録
	particleData.trailPositions[particleData.trailHead] = particleData.transform.translate;
	particleData.trailHead = (particleData.trailHead + 1) % kMaxTrailLength;
	if (particleData.trailCount < kMaxTrailLength) {
		particleData.trailCount++;
	}
}

void TrailModule::CalcTrailPoint(const ParticleData& particleData, uint32_t index, Vector3& outScale, Vector4& outColor) const {
	// 最新が0、最も古いものが1に近づく
	float t = static_cast<float>(index + 1) / static_cast<float>(GetTrailLength());

	float scale = Lerp(startScale_, endScale_, t, easeCurve_);
	outScale = particleData.transform.scale * scale;

	// パーティクルの色から目標の色へHSV空間で補間する
	Vector3 startHsv = Math::RGBtoHSV({ particleData.color.x, particleData.color.y, particleData.color.z });
	Vector3 endHsv = Math::RGBtoHSV({ trailColor_.x, trailColor_.y, trailColor_.z });
	Vector3 hsv = Lerp(startHsv, endHsv, t, easeCurve_);
	Vector3 rgb = Math::HSVtoRGB(hsv.x, std::clamp(hsv.y, 0.0f, 1.0f), std::clamp(hsv.z, 0.0f, 1.0f));

	float alpha = Lerp(startAlpha_, endAlpha_, t, easeCurve_);
	outColor = { rgb.x, rgb.y, rgb.z, particleData.color.w * alpha };
}

uint32_t TrailModule::GetTrailLength() const {
	return std::clamp(trailLength_, 1u, kMaxTrailLength);
}
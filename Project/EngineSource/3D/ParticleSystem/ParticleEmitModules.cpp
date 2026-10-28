#include "pch.h"
#include "ParticleEmitModules.h"
#include "TextureManager.h"
#include "RandomGenerator.h"
#include "MyMath.h"
#include <algorithm>
#include <numbers>
using namespace GameEngine;

//==================================================
// テクスチャモジュール
//==================================================

void TextureModule::SetTexture(TextureManager* textureManager) {
	textureData_.handle = textureManager->GetHandleByName(textureData_.name);
}

//==================================================
// 速度モジュール
//==================================================

void VelocityEmitModule::Create(ParticleData& particleData) {
	particleData.velocity = {
		RandomGenerator::Get(velocityRange_.min.x, velocityRange_.max.x),
		RandomGenerator::Get(velocityRange_.min.y, velocityRange_.max.y),
		RandomGenerator::Get(velocityRange_.min.z, velocityRange_.max.z),
	};
}

//==================================================
// 方向指定速度モジュール
//==================================================

void DirectionEmitModule::Create(ParticleData& particleData) {
	// 基準方向を正規化
	Vector3 baseDir = direction_;
	if (baseDir.LengthSquared() <= 0.0001f) {
		baseDir = { 0.0f, 0.0f, 1.0f };
	} else {
		baseDir = baseDir.Normalize();
	}

	if (spreadAngle_ > 0.0f) {
		// 基準方向と平行にならないup軸を選ぶ
		Vector3 up = (std::fabs(baseDir.Dot(Vector3(0.0f, 1.0f, 0.0f))) > 0.99f) ?
			Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f);
		Vector3 perpendicular = Vector3::Normalize(baseDir.Cross(up));

		// 基準方向を軸に回転させ、広がりの方向を決める
		float spinAngle = RandomGenerator::Get(0.0f, 2.0f * std::numbers::pi_v<float>);
		Quaternion spin = Quaternion::MakeRotateAxisAngleQuaternion(baseDir, spinAngle);
		Vector3 spreadAxis = Quaternion::RotateVector(perpendicular, spin);

		// spreadAxisを軸に基準方向をばらつき角の範囲で傾ける
		float tiltAngle = RandomGenerator::Get(0.0f, spreadAngle_ * (std::numbers::pi_v<float> / 180.0f));
		Quaternion tilt = Quaternion::MakeRotateAxisAngleQuaternion(spreadAxis, tiltAngle);
		baseDir = Quaternion::RotateVector(baseDir, tilt);
	}

	// レンジ対策
	if (maxSpeed_ < minSpeed_) {
		std::swap(minSpeed_, maxSpeed_);
	}

	float speed = RandomGenerator::Get(minSpeed_, maxSpeed_);
	particleData.velocity = baseDir * speed;
}

//==================================================
// 回転モジュール
//==================================================

void RotateEmitModule::Create(ParticleData& particleData) {
	particleData.transform.rotate = {
		RandomGenerator::Get(rotateRange_.min.x, rotateRange_.max.x),
		RandomGenerator::Get(rotateRange_.min.y, rotateRange_.max.y),
		RandomGenerator::Get(rotateRange_.min.z, rotateRange_.max.z),
	};
}

//==================================================
// サイズモジュール
//==================================================

void ScaleEmitModule::Create(ParticleData& particleData) {
	if (separateAxes_) {
		particleData.transform.scale = {
		RandomGenerator::Get(scaleRange_.min.x, scaleRange_.max.x),
		RandomGenerator::Get(scaleRange_.min.y, scaleRange_.max.y),
		RandomGenerator::Get(scaleRange_.min.z, scaleRange_.max.z),
		};
	} else {
		float randomScale = RandomGenerator::Get(scaleRange_.min.x, scaleRange_.max.x);
		particleData.transform.scale = { randomScale, randomScale, randomScale };
	}

	particleData.startSize = particleData.transform.scale;
}

//==================================================
// 回転速度モジュール
//==================================================

void RotateVelocityEmitModule::Create(ParticleData& particleData) {
	particleData.rotateVelocity = {
		RandomGenerator::Get(velocityRange_.min.x, velocityRange_.max.x),
		RandomGenerator::Get(velocityRange_.min.y, velocityRange_.max.y),
		RandomGenerator::Get(velocityRange_.min.z, velocityRange_.max.z),
	};
}

//==================================================
// 発射形状モジュール
//==================================================

void ShapeEmitModule::Create(ParticleData& particleData) {
	Vector3 centerPos = particleData.transform.translate;

	switch (emitterShape_.type) {
	case EmitShapeType::Sphere: {
		Vector3 randomDir;
		while (true) {
			// 立方体の中でランダムに点を取り、球の中に入るまで繰り返す
			randomDir = RandomGenerator::GetVector3(Vector3(-1, -1, -1), Vector3(1, 1, 1));
			if (randomDir.LengthSquared() <= 1.0f && randomDir.LengthSquared() > 0.0001f) {
				break;
			}
		}

		if (emitterShape_.emitFromShell) {
			// 表面
			particleData.transform.translate = centerPos + randomDir.Normalize() * emitterShape_.radius;
		} else {
			// 内部
			particleData.transform.translate = centerPos + randomDir * emitterShape_.radius;
		}
		break;
	}

	case EmitShapeType::Hemisphere: {
		Vector3 randomDir;
		while (true) {
			randomDir = RandomGenerator::GetVector3(Vector3(-1, -1, -1), Vector3(1, 1, 1));
			// 阪急
			if (randomDir.LengthSquared() <= 1.0f && randomDir.LengthSquared() > 0.0001f && randomDir.y >= 0.0f) {
				break;
			}
		}

		if (emitterShape_.emitFromShell) {
			// 表面
			particleData.transform.translate = centerPos + randomDir.Normalize() * emitterShape_.radius;
		} else {
			// 内部
			particleData.transform.translate = centerPos + randomDir * emitterShape_.radius;
		}
		break;
	}

	case EmitShapeType::Box: {
		Vector3 half = emitterShape_.boxSize * 0.5f;
		particleData.transform.translate = RandomGenerator::GetVector3(centerPos - half, centerPos + half);
		break;
	}

	case EmitShapeType::Circle: {
		// XZ平面上の角度をランダムに決める
		float angle = RandomGenerator::Get(0.0f, 2.0f * std::numbers::pi_v<float>);

		float radius = emitterShape_.radius;
		if (!emitterShape_.emitFromShell) {
			// 内部に均等に散らすため、平方根を取ってから半径を掛ける
			radius *= std::sqrt(RandomGenerator::Get(0.0f, 1.0f));
		}

		particleData.transform.translate = centerPos + Vector3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
		break;
	}
	}
}

//==================================================
// 色モジュール
//==================================================

void ColorEmitModule::Create(ParticleData& particleData) {

	Vector3 minHsv = Math::RGBtoHSV({ minColor_.x, minColor_.y, minColor_.z });
	Vector3 maxHsv = Math::RGBtoHSV({ maxColor_.x, maxColor_.y, maxColor_.z });

	if (maxHsv.x < minHsv.x) {
		std::swap(minHsv.x, maxHsv.x);
	}
	if (maxHsv.y < minHsv.y) {
		std::swap(minHsv.y, maxHsv.y);
	}
	if (maxHsv.z < minHsv.z) {
		std::swap(minHsv.z, maxHsv.z);
	}

	Vector4 hsv = {
		RandomGenerator::Get(minHsv.x, maxHsv.x),
		RandomGenerator::Get(minHsv.y, maxHsv.y),
		RandomGenerator::Get(minHsv.z, maxHsv.z),
		RandomGenerator::Get(minColor_.w, maxColor_.w)
	};
	
	Vector3 rgb = Math::HSVtoRGB(hsv.x, std::clamp(hsv.y, 0.0f, 1.0f), std::clamp(hsv.z, 0.0f, 1.0f));

	particleData.color = {
		rgb.x,rgb.y,rgb.z,hsv.w
	};
	particleData.startColor = particleData.color;
}

//==================================================
// 生存時間モジュール
//==================================================

void LifeTimeEmitModule::Create(ParticleData& particleData) {
	// 最小値と最大値が逆に設定されていても動くようにする
	float minLifeTime = minLifeTime_;
	float maxLifeTime = maxLifeTime_;
	if (maxLifeTime < minLifeTime) {
		std::swap(minLifeTime, maxLifeTime);
	}

	// 経過時間の割合を求める時に0除算しないよう、下限を設ける
	particleData.lifeTime = (std::max)(RandomGenerator::Get(minLifeTime, maxLifeTime), 0.01f);
}

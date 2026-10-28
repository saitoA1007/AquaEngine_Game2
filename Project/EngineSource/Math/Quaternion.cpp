#include "pch.h"
#include "Quaternion.h"
#include "MyMath.h"

using namespace GameEngine;

float Quaternion::Norm() const {
	return std::sqrt(x * x + y * y + z * z + w * w);
}

Quaternion Quaternion::Conjugate(const Quaternion& quaternion) {
	return { -quaternion.x, -quaternion.y, -quaternion.z, quaternion.w };
}

float Quaternion::Norm(const Quaternion& quaternion) {
	return std::sqrt(quaternion.x * quaternion.x + quaternion.y * quaternion.y + quaternion.z * quaternion.z + quaternion.w * quaternion.w);
}

Quaternion Quaternion::Normalize(const Quaternion& quaternion) {
	float norm = Norm(quaternion);
	// 0除算を避けるため単位Quaternionを返す
	if (norm == 0.0f) {
		return Quaternion::Identity();
	}
	return { quaternion.x / norm, quaternion.y / norm, quaternion.z / norm, quaternion.w / norm };
}

Quaternion Quaternion::Inverse(const Quaternion& quaternion) {
	float norm = Norm(quaternion);
	// 0除算を避けるため単位Quaternionを返す
	if (norm == 0.0f) {
		return Quaternion::Identity();
	}
	Quaternion conjugate = Conjugate(quaternion);
	float invNorm = 1.0f / (norm * norm);
	return { conjugate.x * invNorm, conjugate.y * invNorm,conjugate.z * invNorm,conjugate.w * invNorm };
}

Quaternion Quaternion::MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle) {
	float halfAngle = angle / 2.0f;
	float sin = std::sin(halfAngle);
	float cos = std::cos(halfAngle);
	return { axis.x * sin, axis.y * sin, axis.z * sin, cos };
}

Vector3 Quaternion::RotateVector(const Vector3& vector, const Quaternion& quaternion) {
	// 四元数とベクトルの回転（q * v * q^-1）
	Quaternion r = { vector.x, vector.y, vector.z,0.0f };
	// quaternionの共役を求める
	Quaternion qConj = Conjugate(quaternion);
	Quaternion rotated = quaternion * r * qConj;
	return { rotated.x, rotated.y, rotated.z };
}

Quaternion Quaternion::MakeEulerQuaternion(float pitch, float yaw, float roll) {
	// pitch==x, yaw==y, roll==z
	Quaternion qx = MakeRotateAxisAngleQuaternion({ 1,0,0 }, pitch);
	Quaternion qy = MakeRotateAxisAngleQuaternion({ 0,1,0 }, yaw);
	Quaternion qz = MakeRotateAxisAngleQuaternion({ 0,0,1 }, roll);

	// 回転順序ZYX
	return qy * (qx * qz);
}

Quaternion Quaternion::DirectionToQuaternion(const Vector3& direction, const Vector3& up) {
	// 基準となる前方向ベクトル
	const Vector3 kForward = { 0.0f, 0.0f, 1.0f };

	// 方向ベクトルを正規化
	float len = direction.Length();
	// 0ベクトル確認
	if (len < 1e-6f) {
		return Quaternion::Identity();
	}
	Vector3 dir = {
		direction.x / len,
		direction.y / len,
		direction.z / len
	};

	float dot = kForward.Dot(dir);

	// 既に同じ方向を向いている場合単位Quaternionを返す
	if (dot >= 1.0f - 1e-6f) {
		return Quaternion::Identity();
	}

	// ほぼ逆方向の場合upベクトルを軸に180度回転させる
	if (dot <= -1.0f + 1e-6f) {
		Vector3 axis = up.Cross(kForward);

		// upも平行な場合はX軸を代替軸にする
		if (axis.Length() < 1e-6f) {
			Vector3 xAxis = { 1.0f, 0.0f, 0.0f };
			axis = xAxis.Cross(kForward);
		}
		axis.Normalize();
		return MakeRotateAxisAngleQuaternion(axis, static_cast<float>(M_PI));
	}

	// kForwardからdirへの回転軸と角度を求める
	Vector3 axis = Vector3::Normalize(kForward.Cross(dir));

	float angle = std::acos(dot);
	return MakeRotateAxisAngleQuaternion(axis, angle);
}
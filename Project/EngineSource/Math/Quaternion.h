#pragma once
#include "Vector3.h"

struct Quaternion {
	float x;
	float y;
	float z;
	float w;

	Quaternion operator+(const Quaternion& other) { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
	Quaternion operator*(const Quaternion& other) const {
		return {
			w * other.x + x * other.w + y * other.z - z * other.y,
			w * other.y - x * other.z + y * other.w + z * other.x,
			w * other.z + x * other.y - y * other.x + z * other.w,
			w * other.w - x * other.x - y * other.y - z * other.z
		};
	}
	Quaternion operator*(const float& other) { return { x * other, y * other, z * other, w * other }; }
	friend Quaternion operator*(float other, const Quaternion& q) { return { q.x * other, q.y * other, q.z * other, q.w * other }; }
	Quaternion operator-() const { return { -x, -y, -z,-w }; }
	Quaternion& operator*=(const Quaternion& other) { *this = *this * other; return *this; }

	// ベクトルの長さ
	float Norm() const;

	float NormSquared() const {
		return x * x + y * y + z * z + w * w;
	}

	// 正規化
	void Normalize() {
		float norm = Norm();
		// ゼロ除算を防ぐためのチェック
		if (norm == 0.0f) {
			Identity();
		} else {
			x /= norm;
			y /= norm;
			z /= norm;
			w /= norm;
		}
	}

	// 内積
	float Dot(const Quaternion& other) const {
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}

	// 共役Quaternionを返す
	void Conjugate() {
		x *= -1.0f;
		y *= -1.0f;
		z *= -1.0f;
	}

	// 初期化
	static Quaternion Identity() {
		return { 0.0f, 0.0f, 0.0f, 1.0f };
	}
	
	// 共役Quaternionを返す
	static Quaternion Conjugate(const Quaternion& quaternion);
	// Quaernionのnormを返す
	static float Norm(const Quaternion& quaternion);
	// 正規化したQuaternionを返す
	static Quaternion Normalize(const Quaternion& quaternion);
	// 逆Quaternionを返す
	static Quaternion Inverse(const Quaternion& quaternion);
	// 任意軸回転行列を表すQuaternionの生成
	static Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle);
	// ベクトルをQuaternionで回転させた結果のベクトルを求める
	static Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion);
	// オイラー角からQuaternionを作成(回転順序ZYX)
	static Quaternion MakeEulerQuaternion(float pitch, float yaw, float roll);
	// 目標ベクトルへへ最短回転
	static Quaternion DirectionToQuaternion(const Vector3& direction, const Vector3& up = { 0.0f, 1.0f, 0.0f });
};
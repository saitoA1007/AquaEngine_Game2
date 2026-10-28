#pragma once
#include <cmath>

struct Vector3 {
	float x, y, z;

	Vector3 operator+(const Vector3& other) const { return { x + other.x, y + other.y, z + other.z }; }
	Vector3 operator-(const Vector3& other) const { return { x - other.x, y - other.y, z - other.z }; }
	Vector3 operator*(const Vector3& other) const { return { x * other.x, y * other.y, z * other.z }; }
	Vector3 operator/(const Vector3& other) const { return { x / other.x, y / other.y, z / other.z }; }
	Vector3 operator+=(const Vector3& other) { return { x += other.x, y += other.y, z += other.z }; }
	Vector3 operator-=(const Vector3& other) { return { x -= other.x, y -= other.y, z -= other.z }; }
	Vector3 operator*=(const Vector3& other) { return { x *= other.x, y *= other.y, z *= other.z }; }
	Vector3 operator/=(const Vector3& other) { return { x /= other.x, y /= other.y, z /= other.z }; }
	Vector3 operator+(const float& other) const { return { x + other, y + other, z + other }; }
	Vector3 operator-(const float& other) const { return { x - other, y - other, z - other }; }
	Vector3 operator*(const float& other) const { return { x * other, y * other, z * other }; }
	Vector3 operator/(const float& other) const { return { x / other, y / other, z / other }; }
	Vector3 operator+=(const float& other) { return { x += other, y += other, z += other }; }
	Vector3 operator-=(const float& other) { return { x -= other, y -= other, z -= other }; }
	Vector3 operator*=(const float& other) { return { x *= other, y *= other, z *= other }; }
	Vector3 operator/=(const float& other) { return { x /= other, y /= other, z /= other }; }

	// ベクトルの長さ
	float Length() const {
		return std::sqrt(x * x + y * y + z * z);
	}

	// ベクトルの長さの2乗
	float LengthSquared() const {
		return x * x + y * y + z * z;
	}

	// 正規化
	Vector3 Normalize() {
		float len = Length();
		// ゼロ除算を防ぐためのチェック
		if (len > 0.0f) {
			x /= len;
			y /= len;
			z /= len;
		}
		return Vector3(x, y, z);
	}

	// 内積
	float Dot(const Vector3& other) const {
		return x * other.x + y * other.y + z * other.z;
	}

	// 外積
	Vector3 Cross(const Vector3& other) const {
		return Vector3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
	}
	
	// 内積
	static float Dot(const Vector3& v1, const Vector3& v2) {
		return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	}

	// 外積
	static Vector3 Cross(const Vector3& v1, const Vector3& v2) {
		return Vector3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
	}

	// 正規化
	static Vector3 Normalize(const Vector3& v) {
		float length = v.Length();
		if (length == 0.0f) {
			return Vector3(0.0f, 0.0f, 0.0f);
		} else {
			return Vector3(v.x / length, v.y / length, v.z / length);
		}
	}

	// 距離の2乗
	static float DistanceSquared(const Vector3& v1, const Vector3& v2) {
		return (v1 - v2).LengthSquared();
	}

	// 距離
	static float Distance(const Vector3& v1, const Vector3& v2) {
		return std::sqrt(DistanceSquared(v1, v2));
	}
};
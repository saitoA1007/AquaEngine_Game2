#pragma once
#include <cmath>

struct Vector2 {
	float x, y;

	Vector2 operator+(const Vector2& other) const { return { x + other.x, y + other.y }; }
	Vector2 operator-(const Vector2& other) const { return { x - other.x, y - other.y }; }
	Vector2 operator*(const Vector2& other) const { return { x * other.x, y * other.y }; }
	Vector2 operator/(const Vector2& other) const { return { x / other.x, y / other.y }; }
	Vector2 operator+=(const Vector2& other) { return { x += other.x, y += other.y }; }
	Vector2 operator-=(const Vector2& other) { return { x -= other.x, y -= other.y }; }
	Vector2 operator*=(const Vector2& other) { return { x *= other.x, y *= other.y }; }
	Vector2 operator/=(const Vector2& other) { return { x /= other.x, y /= other.y }; }
	Vector2 operator+(const float& other) const { return { x + other, y + other }; }
	Vector2 operator-(const float& other) const { return { x - other, y - other }; }
	Vector2 operator*(const float& other) const { return { x * other, y * other }; }
	Vector2 operator/(const float& other) const { return { x / other, y / other }; }

	// ベクトルの長さ
	float Length() const {
		return std::sqrt(x * x + y * y);
	}

	// ベクトルの長さの2乗
	float LengthSquared() const {
		return x * x + y * y;
	}

	// 正規化
	void Normalize() {
		float len = Length();
		// ゼロ除算を防ぐためのチェック
		if (len > 0.0f) {
			x /= len;
			y /= len;
		}
	}

	// 内積
	float Dot(const Vector2& other) const {
		return x * other.x + y * other.y;
	}

	// 正規化
	static Vector2 Normalize(const Vector2& v) {
		float length = v.Length();
		if (length == 0.0f) {
			return Vector2(0.0f, 0.0f);
		} else {
			return Vector2(v.x / length, v.y / length);
		}
	}

	// 距離の2乗
	static float DistanceSquared(const Vector2& v1, const Vector2& v2) {
		return (v1 - v2).LengthSquared();
	}

	// 距離
	static float Distance(const Vector2& v1, const Vector2& v2) {
		return std::sqrt(DistanceSquared(v1, v2));
	}
};

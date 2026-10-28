#pragma once

struct Vector4 {
	float x, y, z, w;

	Vector4 operator+(const Vector4& other) const { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
	Vector4 operator-(const Vector4& other) const { return { x - other.x, y - other.y, z - other.z, w - other.w }; }
	Vector4 operator*(const Vector4& other) const { return { x * other.x, y * other.y, z * other.z, w * other.w }; }
	Vector4 operator/(const Vector4& other) const { return { x / other.x, y / other.y, z / other.z, w / other.w }; }
	Vector4 operator+=(const Vector4& other) { return { x += other.x, y += other.y, z += other.z, w += other.w }; }
	Vector4 operator-=(const Vector4& other) { return { x -= other.x, y -= other.y, z -= other.z, w -= other.w }; }
	Vector4 operator*=(const Vector4& other) { return { x *= other.x, y *= other.y, z *= other.z, w *= other.w }; }
	Vector4 operator/=(const Vector4& other) { return { x /= other.x, y /= other.y, z /= other.z, w /= other.w }; }
	Vector4 operator+(const float& other) const { return { x + other, y + other, z + other, w + other }; }
	Vector4 operator-(const float& other) const { return { x - other, y - other, z - other, w - other }; }
	Vector4 operator*(const float& other) const { return { x * other, y * other, z * other, w * other }; }
	Vector4 operator/(const float& other) const { return { x / other, y / other, z / other, w / other }; }
	Vector4 operator-() const { return { -x, -y, -z, -w }; }

	// ベクトルの長さ
	float Length() const {
		return std::sqrt(x * x + y * y + z * z + w * w);
	}

	// ベクトルの長さの2乗
	float LengthSquared() const {
		return x * x + y * y + z * z + w * w;
	}

	// 正規化
	Vector4 Normalize() {
		float len = Length();
		// ゼロ除算を防ぐためのチェック
		if (len > 0.0f) {
			x /= len;
			y /= len;
			z /= len;
			w /= len;
		}
		return Vector4(x, y, z, w);
	}

	// 内積
	float Dot(const Vector4& other) const {
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}

	// 正規化
	static Vector4 Normalize(const Vector4& v) {
		float length = v.Length();
		if (length == 0.0f) {
			return Vector4(0.0f, 0.0f, 0.0f, 0.0f);
		} else {
			return Vector4(v.x / length, v.y / length, v.z / length, v.w / length);
		}
	}

	// 距離の2乗
	static float DistanceSquared(const Vector4& v1, const Vector4& v2) {
		return (v1 - v2).LengthSquared();
	}

	// 距離
	static float Distance(const Vector4& v1, const Vector4& v2) {
		return std::sqrt(DistanceSquared(v1, v2));
	}
};

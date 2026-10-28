#pragma once
#include "Vector3.h"
#include "Quaternion.h"

struct Matrix4x4 {
	float m[4][4];

	Matrix4x4 operator+(const Matrix4x4& other) {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = m[i][j] + other.m[i][j];
			}
		}
		return result;
	}
	Matrix4x4 operator-(const Matrix4x4& other) {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = m[i][j] - other.m[i][j];
			}
		}
		return result;
	}
	Matrix4x4 operator*(const Matrix4x4& other) const {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = 0;
				for (int k = 0; k < 4; ++k) {
					result.m[i][j] += m[i][k] * other.m[k][j];
				}
			}
		}
		return result;
	}
	Matrix4x4 operator/(const Matrix4x4& other) {
		Matrix4x4 result;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				result.m[i][j] = m[i][j] / other.m[i][j];
			}
		}
		return result;
	}

	Matrix4x4& operator*=(const Matrix4x4& other) {
		*this = *this * other;
		return *this;
	}

	// 初期化
	static Matrix4x4 MakeIdentity() {
		Matrix4x4 matrix = {};
		matrix.m[0][0] = 1.0f;
		matrix.m[1][1] = 1.0f;
		matrix.m[2][2] = 1.0f;
		matrix.m[3][3] = 1.0f;
		return matrix;
	}

	// 4x4のX軸の回転行列を作成
	static Matrix4x4 MakeRotateXMatrix(const float& theta);
	// 4x4のY軸の回転行列を作成
	static Matrix4x4 MakeRotateYMatrix(const float& theta);
	// 4x4のZ軸の回転行列を作成
	static Matrix4x4 MakeRotateZMatrix(const float& theta);

	// 4x4の拡縮行列の作成
	static Matrix4x4 MakeScaleMatrix(const Vector3& scale);

	// 4x4の平行移動行列の作成
	static Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

	// 4x4のSRTによるアフィン変換行列の作成
	static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& theta, const Vector3 translate);
	static Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& quaternion, const Vector3 translate);

	// 4x4行列の任意軸回転行列の作成
	static Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle);

	// Quaternionから回転行列を求める
	static Matrix4x4 MakeRotateMatrix(const Quaternion& q);

	// 透視投影行列の作成
	static Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

	// 平行投射行列の作成
	static Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

	// ビューポート行列の作成
	static Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minD, float maxD);

	// 4x4逆行列の計算
	static Matrix4x4 Inverse(const Matrix4x4& matrix);

	// 4x4行列の転置
	static Matrix4x4 Transpose(const Matrix4x4& matrix);

	// 4x4行列の逆転置行列
	static Matrix4x4 InverseTranspose(const Matrix4x4& matrix);

	// (3+1)次元座標系をデカルト座標系に変換
	static Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

	// ベクトル変換
	static Vector3 TransformNormal(const Vector3& v, const Matrix4x4& matrix);
};
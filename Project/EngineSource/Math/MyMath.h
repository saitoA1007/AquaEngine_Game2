#pragma once
#include"Vector2.h"
#include"Vector3.h"
#include"Vector4.h"
#include"Matrix4x4.h"

#include"Quaternion.h"

static const double M_PI = 3.14159265358979323846;
constexpr float PI = 3.1415926535f;
constexpr float TWO_PI = PI * 2.0f;

namespace GameEngine {

	namespace Math {

		// 方向からオイラー回転を求める
		Vector3 DirectionToEuler(const Vector3& direction);

		// 2つのベクトルからなす角を求める
		float AngleBetweenRadians(Vector3 v1, Vector3 v2);

		// ワールドスクリーン座標変換(ワールド->スクリーン変換)
		Vector3 Project(const Vector3& worldPosition, const Vector2& viewport, const float& viewportWidth, const float& viewportHeight, const Matrix4x4& viewProjection);

		// ラジアン角度から方向ベクトルを求める
		Vector3 PitchToDirection(float pitch);
		Vector3 YawToDirection(float yaw);
		Vector3 RollToDirection(float roll);

		// 最短経路で角度を補間する
		float LerpShortAngle(float a, float b, float t);

		// 補間した差分を求める
		float GetAngleDiff(float a, float b);

		// 0~360度の範囲に抑える
		float WrapAngle(float angle);

		// マウスの位置からレイの方向を取得
		Vector3 CalculateRayDirection(Vector2 mousePos, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, float windowWidth = 1280.0f, float windowHeight = 720.0f);

		// 最大値
		Vector3 Max(Vector3 v1, Vector3 v2);
		Vector4 Max(Vector4 v1, Vector4 v2);
		// 最小値
		Vector3 Min(Vector3 v1, Vector3 v2);
		Vector4 Min(Vector4 v1, Vector4 v2);

		/// <summary>
		/// ビルボードを適応させるためのworldMatrixを作成
		/// </summary>
		/// <param name="scale"></param>
		/// <param name="translate"></param>
		/// <param name="cameraMatrix"></param>
		/// <returns></returns>
		Matrix4x4 MakeBillboardMatrix(const Vector3& scale, const Vector3& translate, const Matrix4x4& cameraMatrix);
		Matrix4x4 MakeBillboardMatrix(const Vector3& scale, const Vector3& translate, float rotateZ, const Matrix4x4& cameraMatrix);

		Matrix4x4 MakeDirectionalBillboardMatrix(const Vector3& scale, const Vector3& translate, const Matrix4x4& cameraMatrix, const Matrix4x4& viewMatrix, const Vector3& velocity, float rotateZ = 0.0f);

		// クウォータニオンによる回転行列を作成
		Matrix4x4 MakeWorldMatrixFromEulerRotation(const Vector3 position, const Vector3& rotateEuler, const Vector3& scale);

		/// <summary>
		/// カメラをターゲットの方向に向かせる
		/// </summary>
		/// <param name="eye">カメラの位置</param>
		/// <param name="center">ターゲットの位置</param>
		/// <param name="up">向き</param>
		/// <returns></returns>
		Matrix4x4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up);

		// rgbをhsvに変換
		Vector3 RGBtoHSV(const Vector3& rgb);

		// hsvをrgbに変換
		Vector3 HSVtoRGB(float h, float s, float v);
	}
}



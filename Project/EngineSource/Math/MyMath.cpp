#include "pch.h"
#include "MyMath.h"
#include <cassert>
#include <cmath>
#include <algorithm>

namespace GameEngine {
	namespace Math {

		Vector3 DirectionToEuler(const Vector3& direction) {
			// 方向ベクトルを正規化
			float len = direction.Length();
			// 0ベクトル確認
			if (len < 1e-6f) {
				return { 0.0f, 0.0f, 0.0f };
			}
			Vector3 dir = {
				direction.x / len,
				direction.y / len,
				direction.z / len
			};

			// Pitch
			float clampedY = std::clamp(dir.y, -1.0f, 1.0f);
			float pitch = -std::asinf(clampedY);

			// Yaw
			float yaw = std::atan2(dir.x, dir.z);

			// Roll
			float roll = 0.0f;

			return { pitch, yaw, roll };
		}

		float AngleBetweenRadians(Vector3 v1, Vector3 v2) {
			v1.Normalize();
			v2.Normalize();

			// 内積を求める
			float dot = v1.Dot(v2);;
			dot = std::clamp(dot, -1.0f, 1.0f);

			// 内積から角度を求める
			return std::acos(dot);
		}

		Vector3 Project(const Vector3& worldPosition, const Vector2& viewport, const float& viewportWidth, const float& viewportHeight, const Matrix4x4& viewProjection) {

			// ビューポート行列
			Matrix4x4 viewportMatrix = Matrix4x4::MakeViewportMatrix(viewport.x, viewport.y, viewportWidth, viewportHeight, 0, 1);
			// ビュー行列とプロジェクション行列、ビューポート行列を合成する
			Matrix4x4 viewProjectionViewportMatrix = viewProjection * viewportMatrix;
			// ワールド->スクリーン座標変換(3Dから2Dへ)
			Vector3 screenPos = Matrix4x4::Transform(worldPosition, viewProjectionViewportMatrix);
			return  screenPos;
		}

		Vector3 PitchToDirection(float pitch) {
			return { 0.0f, std::sinf(pitch), std::cosf(pitch) };
		}

		Vector3 YawToDirection(float yaw) {
			return { std::sinf(yaw), 0.0f, std::cosf(yaw) };
		}

		Vector3 RollToDirection(float roll) {
			return { std::sinf(roll), std::cosf(roll), 0.0f };
		}

		float LerpShortAngle(float a, float b, float t) {
			float diff = b - a;
			// -2pi-2piに補正する
			diff = std::fmodf(diff, TWO_PI);
			// -pi-piに補正する
			if (diff < -PI) {
				diff += TWO_PI;
			} else if (diff > PI) {
				diff -= TWO_PI;
			}
			return a + diff * t;
		}

		float GetAngleDiff(float a, float b) {
			float diff = b - a;

			// -2pi-2piに補正する
			diff = std::fmodf(diff, TWO_PI);

			// -pi ~ piに補正
			if (diff < -PI) {
				diff += TWO_PI;
			} else if (diff > PI) {
				diff -= TWO_PI;
			}

			// 微小差分判定
			if (std::fabsf(diff) < 1.0e-4f) {
				return 0.0f;
			}

			return diff;
		}

		float WrapAngle(float angle) {
			angle = std::fmod(angle, TWO_PI);
			if (angle < 0.0f) {
				angle += TWO_PI;
			}
			return angle;
		}

		Vector3 CalculateRayDirection(Vector2 mousePos, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, float windowWidth, float windowHeight) {

			float ndcX = (2.0f * mousePos.x) / windowWidth - 1.0f;
			float ndcY = 1.0f - (2.0f * mousePos.y) / windowHeight;

			// ビュー空間へ変換
			Matrix4x4 invProj = Matrix4x4::Inverse(projectionMatrix);
			Vector3 nearView = Matrix4x4::Transform(Vector3(ndcX, ndcY, 0.0f), invProj);
			Vector3 farView = Matrix4x4::Transform(Vector3(ndcX, ndcY, 1.0f), invProj);

			// ビュー空間でのレイ方向
			Vector3 rayView = farView - nearView;
			rayView.Normalize();

			// ビュー行列の逆行列を掛けて、ワールド空間へ変換
			Matrix4x4 invView = Matrix4x4::Inverse(viewMatrix);
			Vector3 rayWorld = Matrix4x4::TransformNormal(rayView, invView);

			return rayWorld.Normalize();
		}

		Vector3 Max(Vector3 v1, Vector3 v2) {
			return Vector3(std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z));
		}

		Vector4 Max(Vector4 v1, Vector4 v2) {
			return Vector4(std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z), std::max(v1.w, v2.w));
		}

		Vector3 Min(Vector3 v1, Vector3 v2) {
			return Vector3(std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z));
		}

		Vector4 Min(Vector4 v1, Vector4 v2) {
			return Vector4(std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z), std::min(v1.w, v2.w));
		}

		Matrix4x4 Multiply(const Matrix4x4& matrix1, const Matrix4x4& matrix2) {
			Matrix4x4 result;
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					result.m[i][j] = 0;
					for (int k = 0; k < 4; ++k) {
						result.m[i][j] += matrix1.m[i][k] * matrix2.m[k][j];
					}
				}
			}
			return result;
		}

		Matrix4x4 MakeBillboardMatrix(const Vector3& scale, const Vector3& translate, const Matrix4x4& cameraMatrix) {

			// ビルボードの回転行列を作成
			Matrix4x4 backToFrontMatrix = Matrix4x4::MakeRotateYMatrix(0.0f);
			Matrix4x4 billboardMatrix = Math::Multiply(backToFrontMatrix, cameraMatrix);
			billboardMatrix.m[3][0] = 0.0f;
			billboardMatrix.m[3][1] = 0.0f;
			billboardMatrix.m[3][2] = 0.0f;
			// ST行列を作成
			Matrix4x4 scaleMatrix = Matrix4x4::MakeScaleMatrix(scale);
			Matrix4x4 translateMatrix = Matrix4x4::MakeTranslateMatrix(translate);
			// 行列の更新
			return scaleMatrix * billboardMatrix * translateMatrix;
		}

		Matrix4x4 MakeBillboardMatrix(const Vector3& scale, const Vector3& translate, float rotateZ, const Matrix4x4& cameraMatrix) {

			// スケール行列
			Matrix4x4 scaleMatrix = Matrix4x4::MakeScaleMatrix(scale);

			// パーティクル自体のローカル回転行列を作成
			Matrix4x4 localRotateMatrix = Matrix4x4::MakeRotateZMatrix(rotateZ);

			// ビルボードの回転行列を作成
			Matrix4x4 backToFrontMatrix = Matrix4x4::MakeRotateYMatrix(0.0f);
			Matrix4x4 billboardMatrix = Math::Multiply(backToFrontMatrix, cameraMatrix);
			billboardMatrix.m[3][0] = 0.0f;
			billboardMatrix.m[3][1] = 0.0f;
			billboardMatrix.m[3][2] = 0.0f;

			// 平行移動行列の作成
			Matrix4x4 translateMatrix = Matrix4x4::MakeTranslateMatrix(translate);

			// 行列の更新
			return scaleMatrix * localRotateMatrix * billboardMatrix * translateMatrix;
		}

		Matrix4x4 MakeDirectionalBillboardMatrix(const Vector3& scale, const Vector3& translate, const Matrix4x4& cameraMatrix, const Matrix4x4& viewMatrix, const Vector3& velocity, float rotateZ) {
			// 1. ビルボード行列（カメラの回転をコピーしてZ軸回転などをリセット）
			Matrix4x4 backToFrontMatrix = Matrix4x4::MakeRotateYMatrix(0.0f);
			Matrix4x4 billboardMatrix = Multiply(backToFrontMatrix, cameraMatrix);
			billboardMatrix.m[3][0] = 0.0f;
			billboardMatrix.m[3][1] = 0.0f;
			billboardMatrix.m[3][2] = 0.0f;

			Vector3 viewVel;
			viewVel.x = velocity.x * viewMatrix.m[0][0] + velocity.y * viewMatrix.m[1][0] + velocity.z * viewMatrix.m[2][0];
			viewVel.y = velocity.x * viewMatrix.m[0][1] + velocity.y * viewMatrix.m[1][1] + velocity.z * viewMatrix.m[2][1];

			// 回転行列を成分から作成
			Matrix4x4 rotateMatrix = Matrix4x4::MakeIdentity();

			// ベクトルの長さの二乗
			float lengthSq = viewVel.x * viewVel.x + viewVel.y * viewVel.y;

			// 速度が十分にある場合のみ向きを変える
			if (lengthSq > 0.000001f) {
				// 正規化係数 (1 / √lengthSq)
				float invLength = 1.0f / std::sqrtf(lengthSq);

				// 正規化された成分 = cosθ, sinθ に相当
				float cosTheta = viewVel.y * invLength; // Y軸基準なのでYがCos相当
				float sinTheta = viewVel.x * invLength; // Y軸基準なのでXがSin相当

				rotateMatrix.m[0][0] = cosTheta;
				rotateMatrix.m[0][1] = -sinTheta;
				rotateMatrix.m[1][0] = sinTheta;
				rotateMatrix.m[1][1] = cosTheta;
			}

			// 最初に設定されたZ回転を維持するためのローカル回転
			Matrix4x4 localRotateMatrix = Matrix4x4::MakeRotateZMatrix(rotateZ);

			Matrix4x4 scaleMatrix = Matrix4x4::MakeScaleMatrix(scale);
			Matrix4x4 translateMatrix = Matrix4x4::MakeTranslateMatrix(translate);

			return scaleMatrix * localRotateMatrix * rotateMatrix * billboardMatrix * translateMatrix;
		}

		Matrix4x4 MakeWorldMatrixFromEulerRotation(const Vector3 position, const Vector3& rotateEuler, const Vector3& scale) {

			// 回転行列を作成
			Quaternion rotate = Quaternion::MakeEulerQuaternion(rotateEuler.x, rotateEuler.y, rotateEuler.z);
			Matrix4x4 rotateMatrix = Matrix4x4::MakeRotateMatrix(rotate);

			// 拡縮行列
			Matrix4x4 scaleMatrix = {
				scale.x, 0.0f,   0.0f,   0.0f,
				0.0f,   scale.y, 0.0f,   0.0f,
				0.0f,   0.0f,   scale.z, 0.0f,
				0.0f,   0.0f,   0.0f,    1.0f
			};

			// 平行移動行列
			Matrix4x4 translateMatrix = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				position.x, position.y, position.z, 1.0f
			};

			// SRT行列
			Matrix4x4 worldMatrix = (scaleMatrix * rotateMatrix) * translateMatrix;
			return worldMatrix;
		}

		Matrix4x4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {

			// カメラの方向ベクトル
			Vector3 z = Vector3::Normalize(center - eye); // 前方向ベクトル
			Vector3 x = Vector3::Normalize(up.Cross(z)); // 右方向ベクトル
			Vector3 y = z.Cross(x);             // 上方向ベクトル

			float tx = x.Dot(eye);
			float ty = y.Dot(eye);
			float tz = z.Dot(eye);

			Matrix4x4 result = { {
				{ x.x,  x.y,  x.z,  0.0f },
				{ y.x,  y.y,  y.z,  0.0f },
				{ z.x,  z.y,  z.z,  0.0f },
				{ tx,   ty,   tz,   1.0f }
			} };
			return result;
		}

		Vector3 RGBtoHSV(const Vector3& rgb) {
			float maxVal = std::fmaxf(rgb.x, std::fmaxf(rgb.y, rgb.z));
			float minVal = std::fminf(rgb.x, std::fminf(rgb.y, rgb.z));
			float delta = maxVal - minVal;

			float h = 0.0f;
			float s = (maxVal > 0.0f) ? (delta / maxVal) : 0.0f;
			float v = maxVal;

			if (delta > 0.0f) {
				if (maxVal == rgb.x) {
					h = (rgb.y - rgb.z) / delta;
					if (h < 0.0f) h += 6.0f;
				} else if (maxVal == rgb.y) {
					h = (rgb.z - rgb.x) / delta + 2.0f;
				} else {
					h = (rgb.x - rgb.y) / delta + 4.0f;
				}
				h /= 6.0f;
			}

			return { h, s, v };
		}

		Vector3 HSVtoRGB(float h, float s, float v) {
			h = h - std::floorf(h);
			float i = std::floorf(h * 6.0f);
			float f = h * 6.0f - i;
			float p = v * (1.0f - s);
			float q = v * (1.0f - f * s);
			float t = v * (1.0f - (1.0f - f) * s);
			switch (static_cast<int>(i) % 6) {
			case 0: return { v, t, p };
			case 1: return { q, v, p };
			case 2: return { p, v, t };
			case 3: return { p, q, v };
			case 4: return { t, p, v };
			default:return { v, p, q };
			}
		}
	}
}
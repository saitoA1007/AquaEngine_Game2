#include "pch.h"
#include"DirectionalLight.h"
#include"EngineSource/Math/MyMath.h"
#include<cmath>
using namespace GameEngine;

void DirectionalLight::Initialize(const Vector4& color,const Vector3& direction,const float& intensity) {
	// デフォルト値を設定
	directionalLightData_.color = color;// 色
	directionalLightData_.direction = direction;// 方向
	directionalLightData_.intensity = intensity;// 輝度
	directionalLightData_.active = false;
	directionalLightData_.isDepthTexture = 0;
}

void DirectionalLight::SetLightDir(const Vector3& lightdir) {
	directionalLightData_.direction = Vector3::Normalize(lightdir);
}

void DirectionalLight::CreateDirectionalShadowMatrix(const Vector3& targetCenter,float shadowRange) {

    //=================================================
    // 
    // 
    // 回転からライトのベクトルを求める方式に変更する
    // 
    // 
    //=================================================

    // ライトの方向を正規化
    directionalLightData_.direction.Normalize();
    Vector3 lightDir = directionalLightData_.direction;
   
    // View行列の作成
    float distance = shadowRange * 2.0f;
    Vector3 lightPos = targetCenter + (lightDir * distance);
   
    Vector3 candidateUps[] = {
        Vector3(0, 1, 0),   // Y-up（デフォルト）
        Vector3(0, 0, 1),   // Z-forward
        Vector3(1, 0, 0)    // X-right
    };

    Vector3 worldUp;
    float minDot = 1.0f;

    // ライト方向と最も垂直に近い軸を選択
    for (int i = 0; i < 3; ++i) {
        float dotProduct = std::abs(lightDir.Dot(candidateUps[i]));
        if (dotProduct < minDot) {
            minDot = dotProduct;
            worldUp = candidateUps[i];
        }
    }

    // 右ベクトルを計算
    Vector3 right = Vector3::Normalize(worldUp.Cross(lightDir));
    // ライトと右ベクトル直交ベクトルを求める
    Vector3 up = Vector3::Normalize(lightDir.Cross(right));
   
    Matrix4x4 worldMatrix = Math::LookAt(lightPos, targetCenter, up);

    // Projection行列の作成
    float r = shadowRange;
    float l = -shadowRange;
    float t = shadowRange;
    float b = -shadowRange;
    float nearPlane = 0.1f;
    float farPlane = distance * 2.5f;
    Matrix4x4 projMatrix = Matrix4x4::MakeOrthographicMatrix(l, t, r, b, nearPlane, farPlane);
   
    // シャドウマップのチラつきを補正
    Matrix4x4 vpMatrix = worldMatrix * projMatrix;

    directionalLightData_.vpMatrix = vpMatrix;
   
    // ワールド原点をシャドウマップ空間へ変換
    Vector3 shadowOrigin = { 0.0f, 0.0f, 0.0f};
    shadowOrigin = Matrix4x4::Transform(shadowOrigin, vpMatrix);
   
    // シャドウマップのサイズ
    float shadowMapSize = 2048.0f;
    // シャドウマップの1テクセルあたりの大きさ
    float texelSize = 2.0f / shadowMapSize;
   
    // ズレを計算
    float offsetX = shadowOrigin.x - (std::floor(shadowOrigin.x / texelSize) * texelSize);
    float offsetY = shadowOrigin.y - (std::floor(shadowOrigin.y / texelSize) * texelSize);
   
    // 射影行列をテクセル分だけ逆にずらす
    projMatrix.m[3][0] -= offsetX;
    projMatrix.m[3][1] -= offsetY;
   
    //directionalLightData_.vpMatrix = worldMatrix * projMatrix;
}
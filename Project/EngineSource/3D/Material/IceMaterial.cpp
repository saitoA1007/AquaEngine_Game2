#include "pch.h"
#include "IceMaterial.h"
#include "MyMath.h"
using namespace GameEngine;

IceMaterial::IceMaterial() {
	// マテリアルデータを作成
	materialBuffer_.CreateTypeless();
	materialData_ = materialBuffer_.GetData();

	// 白色に設定
	materialData_->color = Vector4(1, 1, 1, 1);
	// Lightingするのでtrueに設定する
	materialData_->enableLighting = true;
	// UVTransform行列を初期化
	materialData_->uvTransform = Matrix4x4::MakeIdentity();
	// specularの色を設定
	materialData_->specularColor = Vector4(1, 1, 1, 1);
	// 輝度を設定
	materialData_->shininess = 500.0f;
	// テクスチャデータ
	materialData_->textureHandle = 0;
	// 環境光
	materialData_->metallic = 0.01f;
	// 影の適応
	materialData_->isActiveShadow = false;
	// 屈折率
	materialData_->ior = 1.31f;
	// 粗さの設定
	materialData_->roughness = 0.5f;
	// ノーマルマップ用のテクスチャ
	materialData_->normalTextureHandle = 0;
	// ディゾルブ用のテクスチャ
	materialData_->dissolveTextureHandle = 0;
	// ディゾルブ用の閾値
	materialData_->dissolveThreshold = 0.45f;

	materialData_->chipScale = 0.23f;
	materialData_->chipStrength = 0.66f;
	materialData_->edgeWidth = 0.46f;
	materialData_->edgeStrength = 0.6f;
	materialData_->microScale = 0.01f;
	materialData_->microStrength = 0.24f;

	materialData_->heightTextureHandle = 0;
	materialData_->heightScale = 0.001f;

	materialData_->bubbleScale = 0.1f;
	materialData_->bubbleMaxDepth = 0.5f;
	materialData_->bubbleDensity = 0.03f;
	materialData_->bubbleJitter = 0.4f;
	materialData_->bubbleHighlight = 0.15f;

	materialData_->rimColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_->rimIntensity = 1.0f;
	materialData_->rimPower = 5.0f;
}

IceMaterial::~IceMaterial() {

}
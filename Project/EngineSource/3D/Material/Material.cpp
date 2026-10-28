#include "pch.h"
#include "Material.h"
#include "MyMath.h"
using namespace GameEngine;

Material::~Material() {
	
}

void Material::Initialize(const Vector4& color, const Vector3& specularColor,const float& shininess,const bool& isEnableLighting) {
	// マテリアルデータを作成
	materialBuffer_.CreateTypeless();
	materialData_ = materialBuffer_.GetData();

	// 白色に設定
	materialData_->color = color;
	// Lightingするのでtrueに設定する
	materialData_->enableLighting = isEnableLighting;
	// UVTransform行列を初期化
	materialData_->uvTransform = Matrix4x4::MakeIdentity();
	// specularの色を設定
	materialData_->specularColor = Vector4(specularColor.x, specularColor.y, specularColor.z, 1);
	// 輝度を設定
	materialData_->shininess = shininess;
	// テクスチャデータ
	materialData_->textureHandle = 0;
	// 環境光
	materialData_->metallic = 0.01f;
	// 影の適応
	materialData_->isActiveShadow = false;
	// 屈折率
	materialData_->ior = 1.0f;
	// 粗さの設定
	materialData_->roughness = std::sqrt(2.0f / (shininess + 2.0f));
	// ノーマルマップ用のテクスチャ
	materialData_->normalTextureHandle = 0;
	// ディゾルブ用のテクスチャ
	materialData_->dissolveTextureHandle = 0;
	// ディゾルブ用の閾値
	materialData_->dissolveThreshold = 0.5f;
}

void Material::SetUVTransform(Transform uvTransform) {
	materialData_->uvTransform = Matrix4x4::MakeAffineMatrix(uvTransform.scale, uvTransform.rotate, uvTransform.translate);
}
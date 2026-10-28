#pragma once
#include "Vector4.h"
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Transform.h"
#include "StructuredBuffer.h"

namespace GameEngine {

	class Material {
	public:
		struct alignas(16) MaterialData {
			Vector4 color;

			int32_t enableLighting;
			float dissolveThreshold;
			float padding[2];

			Matrix4x4 uvTransform;

			Vector4 specularColor;

			float shininess;
			uint32_t textureHandle;
			float metallic;
			int32_t isActiveShadow;
			float ior; // 屈折率
			float roughness; // 粗さ
			uint32_t normalTextureHandle;
			uint32_t dissolveTextureHandle; // ディゾルブテクスチャ
		};
	public:
		Material() = default;
		~Material();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="color">物体の色</param>
		/// <param name="specularColor">specularの色</param>
		/// <param name="shininess">輝度</param>
		/// <param name="isEnableLighting">ライトを有効化</param>
		void Initialize(const Vector4& color, const Vector3& specularColor, const float& shininess, const bool& isEnableLighting);

	public:

		/// <summary>
		/// 色を設定
		/// </summary>
		/// <param name="color"></param>
		void SetColor(Vector4 color) {
			materialData_->color = color;
		}

		/// <summary>
		/// specularの色を設定
		/// </summary>
		/// <param name="specularColor"></param>
		void SetSpecularColor(Vector3 specularColor) { materialData_->specularColor = Vector4(specularColor.x, specularColor.y, specularColor.z,1); }

		/// <summary>
		/// 透明度を設定
		/// </summary>
		/// <param name="alpha"></param>
		void SetAlpha(const float& alpha) { materialData_->color.w = alpha; }

		/// <summary>
		/// 輝度を設定
		/// </summary>
		/// <param name="shininess"></param>
		void SetShininess(const float& shininess) { materialData_->shininess = shininess; }

		/// <summary>
		/// ライトを適応させるかを設定
		/// </summary>
		/// <param name="isEnableLighting"></param>
		void SetEnableLighting(bool isEnableLighting) { materialData_->enableLighting = isEnableLighting; }

		/// <summary>
		/// 影の適応
		/// </summary>
		/// <param name="isEnableLighting"></param>
		void SetEnableShadow(bool isEnableShadow) { materialData_->isActiveShadow = isEnableShadow; }

		/// <summary>
		/// UV行列を設定
		/// </summary>
		/// <param name="uvMatrix"></param>
		void SetUVMatrix(Matrix4x4 uvMatrix) { materialData_->uvTransform = uvMatrix; }

		/// <summary>
		/// uvトランスフォームを設定
		/// </summary>
		/// <param name="uvTransform"></param>
		void SetUVTransform(Transform uvTransform);

		/// <summary>
		/// 光沢の設定
		/// </summary>
		/// <param name="metallic"></param>
		void SetMetallic(const float& metallic) { materialData_->metallic = metallic; }

		/// <summary>
		/// 粗さを設定。PBRライティングで使用するパラメータ
		/// </summary>
		/// <param name="roughness"></param>
		void SetRoughness(const float& roughness) { materialData_->roughness = roughness; }

		/// <summary>
		/// 屈折率を設定。レイトレで使用するパラメータ
		/// </summary>
		/// <param name="ior"></param>
		void SetIOR(const float& ior) { materialData_->ior = ior; }

		/// <summary>
		/// ディゾルブ用の閾値を設定
		/// </summary>
		/// <param name="threshold"></param>
		void SetDissolveThreshold(float threshold) { materialData_->dissolveThreshold = threshold; }

		/// <summary>
		/// ノーマルマップのテクスチャを設定
		/// </summary>
		/// <param name="texture"></param>
		void SetNormalTexture(const uint32_t& texture) { materialData_->normalTextureHandle = texture; }

		/// <summary>
		/// ディゾルブのテクスチャを設定
		/// </summary>
		/// <param name="texture"></param>
		void SetDissolveTexture(const uint32_t& texture) { materialData_->dissolveTextureHandle = texture; }

		void SetTextureHandle(const uint32_t& tex) {materialData_->textureHandle = tex;}

		const uint32_t& GetTextureHandle() const { return materialData_->textureHandle; }

		void SetDefaultTexture(const uint32_t& handle) { defaultTextureHandle_ = handle; }

		void AdaptDefaultTexture() { materialData_->textureHandle = defaultTextureHandle_; }

		MaterialData* GetMaterialData() { return materialData_; }

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return materialBuffer_.GetGpuVirtualAddress(); }
		StructuredBuffer<MaterialData>& GetMaterialBuffer() { return materialBuffer_; }

		const uint32_t& GetMaterialSrvIndex() const { return materialBuffer_.GetSrvIndex(); }

		void SetTextureName(const std::string& name) {
			textureName_ = name;
		}

		const std::string& GetTextureName() const { return textureName_; }

	private:
		// マテリアルデータ
		StructuredBuffer<MaterialData> materialBuffer_;

		// マテリアルにデータを書き込む
		MaterialData* materialData_ = nullptr;

		// テクスチャの名前
		std::string textureName_ = "";

		// テクスチャ情報
		uint32_t defaultTextureHandle_ = 0;
	};
}
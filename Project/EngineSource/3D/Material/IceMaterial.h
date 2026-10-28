#pragma once
#pragma once
#include "Vector4.h"
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Transform.h"
#include "StructuredBuffer.h"

namespace GameEngine {

	class IceMaterial {
	public:
		struct alignas(16) IceMaterialData {
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

			float chipScale;
			float chipStrength;
			float edgeWidth;
			float edgeStrength;

			float microScale;
			float microStrength;
			uint32_t heightTextureHandle; // ハイトテクスチャ
			float heightScale; // ハイトの高さ

			float bubbleScale; // 気泡セルのスケール
			float bubbleMaxDepth; // 探索する最大深度
			float bubbleDensity; // 気泡の出現確率
			float bubbleJitter; // 気泡位置のばらつき

			float bubbleHighlight; // ハイライト強度
			float rimIntensity; // リムライトの明るさ
			float rimPower; // リムライトの力
			float padding1;

			Vector4 rimColor; // リムライトの色
		};

	public:
		IceMaterial();
		~IceMaterial();

		IceMaterialData* GetMaterialData() { return materialData_; }

		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return materialBuffer_.GetGpuVirtualAddress(); }
		StructuredBuffer<IceMaterialData>& GetMaterialBuffer() { return materialBuffer_; }

		const uint32_t& GetMaterialSrvIndex() const { return materialBuffer_.GetSrvIndex(); }

	public:

		// マテリアルにデータを書き込む
		IceMaterialData* materialData_ = nullptr;

	private:
		// マテリアルデータ
		StructuredBuffer<IceMaterialData> materialBuffer_;
	};
}
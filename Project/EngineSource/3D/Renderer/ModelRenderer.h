#pragma once
#include <d3d12.h>
#include <wrl.h>

#include "SrvManager.h"

#include "Model.h"
#include "WorldTransform.h"
#include "WorldTransforms.h"
#include "FractureInstance.h"

namespace GameEngine {

	class ModelRenderer {
	public:
		ModelRenderer() = default;
		~ModelRenderer() = default;

		/// <summary>
		/// 静的初期化
		/// </summary>
		/// <param name="commandList"></param>
		static void StaticInitialize(ID3D12GraphicsCommandList* commandList,SrvManager* srvManager);

		/// <summary>
		/// カメラを設定する
		/// </summary>
		/// <param name="vpMatrix"></param>
		/// <param name="cameraResource"></param>
		static void SetCamera(ID3D12Resource* cameraResource);

	public:

		/// <summary>
		/// 生成したモデルを描画
		/// </summary>
		/// <param name="worldMatrix">ワールド行列</param>
		/// <param name="directionalLightResource">光源</param>
		/// <param name="material">マテリアル : 何の書かなければデフォルトのマテリアルを適応</param>
		static void Draw(const Model* model, WorldTransform& worldTransform, const GpuResource* material = nullptr);

		/// <summary>
		/// 生成したモデルの描画(ライト適応、複数マテリアル対応)
		/// </summary>
		/// <param name="worldTransform"></param>
		/// <param name="VPMatrix"></param>
		/// <param name="lightGroupResource"></param>
		/// <param name="cameraResource"></param>
		/// <param name="material"></param>
		static void Draw(const Model* model, WorldTransform& worldTransform, GpuResource* lightGroupResource, const GpuResource* material = nullptr);

		/// <summary>
		/// 生成したモデルの複数描画
		/// </summary>
		/// <param name="worldTransforms"></param>
		/// <param name="textureHandle"></param>
		/// <param name="VPMatrix"></param>
		/// <param name="material"></param>
		static void DrawInstancing(const Model* model, const uint32_t& numInstance, WorldTransforms& worldTransforms, const GpuResource* material = nullptr);

		static void DrawWboitInstancing(const Model* model, const uint32_t& numInstance, WorldTransforms& worldTransforms, const GpuResource* wboit,const GpuResource* material = nullptr);

		static void DrawParticleCS(const Model* model, const uint32_t& numInstance, const SrvResource* particle, const GpuResource* camera);

		// 破片を描画
		static void DrawFracture(const Model* model, FractureInstance& fractureInstance, ID3D12CommandSignature* sig, GpuResource* lightGroupResource, const GpuResource* material = nullptr);
		static void DrawRuntimeCutFragments(FractureInstance& fractureInstance, ID3D12CommandSignature* sig, GpuResource* lightGroupResource, const GpuResource* material);

		/// <summary>
		/// グリッドを描画
		/// </summary>
		static void DrawGrid(const Model* model, WorldTransform& worldTransform);

		/// <summary>
		/// デバック用のライン描画
		/// </summary>
		/// <param name="vertexView"></param>
		static void DrawDebugLine(const D3D12_VERTEX_BUFFER_VIEW& vertexView,const uint32_t& totalVertices);

		/// <summary>
		/// アニメーションのあるモデルを描画
		/// </summary>
		/// <param name="worldTransform"></param>
		/// <param name="VPMatrix"></param>
		/// <param name="material"></param>
		static void DrawAnimation(const Model* model, WorldTransform& worldTransform, GpuResource* lightGroupResource,const GpuResource* material = nullptr);

		/// <summary>
		/// モデルに光源を適応させる
		/// </summary>
		/// <param name="directionalLightResource"></param>
		static void DrawLight(GpuResource* lightGroupResource);

		/// <summary>
		/// スカイボックスの描画
		/// </summary>
		/// <param name="model"></param>
		/// <param name="worldTransform"></param>
		/// <param name="material"></param>
		static void DrawSkybox(const Model* model, WorldTransform& worldTransform, const GpuResource* material = nullptr);

		/// <summary>
		/// シャドウマップ用の描画処理
		/// </summary>
		/// <param name="model"></param>
		/// <param name="worldTransform"></param>
		static void DrawShadowMap(const Model* model, WorldTransform& worldTransform);

		/// <summary>
		/// レイトレーシングで描画
		/// </summary>
		/// <param name="model"></param>
		/// <param name="worldTransform"></param>
		static void DrawRaytracing(const Model* model, WorldTransform& worldTransform);

	private:

		// コマンドリスト
		static ID3D12GraphicsCommandList* commandList_;

		// srv
		static SrvManager* srvManager_;

		// カメラリソース
		static ID3D12Resource* cameraResource_;
	};
}

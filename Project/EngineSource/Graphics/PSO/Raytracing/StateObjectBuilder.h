#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include "Externals/DirectXTex/d3dx12.h"

namespace GameEngine {

	class StateObjectBuilder {
	public:

		// 初期化
		void Initialize(D3D12_STATE_OBJECT_TYPE type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

		/// <summary>
		/// シェーダーライブラリを追加する
		/// </summary>
		/// <param name="blob"></param>
		/// <param name="exportNames"></param>
		void AddDXILLibrary(IDxcBlob* blob, const std::vector<std::wstring>& exportNames);

		/// <summary>
		/// ヒットグループを追加
		/// </summary>
		/// <param name="hitGroupName">ヒットグループ名</param>
		/// <param name="closestHitShaderName">衝突した時に呼び出す関数名</param>
		/// <param name="anyHitShaderName">関数名</param>
		/// <param name="intersectionShaderName">関数名</param>
		/// <param name="type"></param>
		void AddHitGroup(
			const std::wstring& hitGroupName,
			const std::wstring& closestHitShaderName,
			const std::wstring& anyHitShaderName = L"",
			const std::wstring& intersectionShaderName = L"",
			D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES);

		/// <summary>
		/// レイのペイロードサイズと属性サイズを設定する
		/// </summary>
		/// <param name="maxPayloadSizeInBytes">ペイロードサイズ</param>
		/// <param name="maxAttributeSizeInBytes">属性サイズ</param>
		void SetShaderConfig(const UINT& maxPayloadSizeInBytes,const UINT& maxAttributeSizeInBytes);

		/// <summary>
		/// レイの最大深度を設定する。最大31回まで。
		/// </summary>
		/// <param name="maxTraceRecursionDepth">レイの最大深度</param>
		void SetPipelineConfig(const UINT& maxTraceRecursionDepth);

		/// <summary>
		/// グローバルルートシグネチャを設定する
		/// </summary>
		/// <param name="rootSignature"></param>
		void SetGlobalRootSignature(ID3D12RootSignature* rootSignature);

		/// <summary>
		/// ローカルルートシグネチャを設定する。指定したシェーダーと連携する。
		/// </summary>
		/// <param name="rootSignature">ローカルルートシグネチャ</param>
		/// <param name="shaderNames">名前</param>
		void AddLocalRootSignature(ID3D12RootSignature* rootSignature, const std::vector<std::wstring>& shaderNames);

		// StateObjectを作成する
		Microsoft::WRL::ComPtr<ID3D12StateObject> Build(ID3D12Device5* device);

	private:
		CD3DX12_STATE_OBJECT_DESC desc_;

		// AddLocalRootSignature 内で生成した文字列ポインターを
		// desc_ が参照するため、寿命を保持する必要がある
		std::vector<std::vector<LPCWSTR>> exportAssociationStorage_;
	};
}

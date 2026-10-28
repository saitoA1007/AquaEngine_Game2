#pragma once
#include "RayLibShaderCompiler.h"
#include "StateObjectBuilder.h"
#include "ShaderTableBuilder.h"
#include "SrvManager.h"

namespace GameEngine {

	namespace AppHitGroups {
		static const std::wstring DefaultModel = L"DefaultModel";
		static const std::wstring IceModel = L"IceModel";
	}

	class RaytracingPipeline {
	public:
		RaytracingPipeline() = default;
		~RaytracingPipeline() = default;

		// 初期化
		void Initialize(ID3D12Device5* device, SrvManager* srvManager, DXC* dxc);

		// シェーダーテーブルを作成
		void CreateShaderTable();

	public:

		const D3D12_DISPATCH_RAYS_DESC& GetDispatchRayDesc() const { return dispatchRayDesc_; }

		ID3D12StateObject* GetStateObject() const { return stateObject_.Get(); }

		ID3D12RootSignature* GetGlobalRootSignature() const {return rootSignatureGlobal_.Get();}

	private:
		ID3D12Device5* device_ = nullptr;
		// srv管理機能
		SrvManager* srvManager_ = nullptr;

		// レイトレーシング用のhlslをコンパイルする機能
		RayLibShaderCompiler rayLibShaderCompiler_;

		// ステートオブジェクトの生成機能
		StateObjectBuilder stateObjectBuilder_;
		Microsoft::WRL::ComPtr<ID3D12StateObject> stateObject_;

		// シェーダーテーブル作成機能
		ShaderTableBuilder shaderTableBuilder_;

		// レイトレーシングを開始する時に衣装する構造体
		D3D12_DISPATCH_RAYS_DESC dispatchRayDesc_;

		// ルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureGlobal_;

	private:

		// グローバルルートシグネチャを作成
		void CreateGlobalRootsignature();

		// ステートオブジェクトを作成
		void CreateStateObject();
	};
}
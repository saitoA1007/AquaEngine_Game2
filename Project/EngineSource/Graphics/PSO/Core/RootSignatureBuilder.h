#pragma once
#include <d3d12.h>
#include <dxcapi.h>
#include <vector>
#include <string>
#include <wrl.h>
#include <map>

namespace GameEngine {

	// パラメータタイプを設定した
	enum class ParameterType {
		CBV,
		SRV,
		UAV,
		RootConstant,
	};

	// パラメータの情報を保持するデータ
	struct ResourceInfo {
		D3D12_SHADER_VISIBILITY visibility;
		uint32_t bindPoint;
		uint32_t space;
		uint32_t bindCount; // SRV用
	};

	class RootSignatureBuilder {
	public:

		struct PendingTable {
			UINT paramIndex; // rootParameters
			UINT rangeIndex; // descriptorRanges
		};

	public:

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="device"></param>
		void Initialize(ID3D12Device* device);

		/// <summary>
		/// cbvを追加する
		/// </summary>
		/// <param name="shaderRegister">レジスタ番号</param>
		/// <param name="visibility">使用するシェーダー</param>
		/// <param name="space"></param>
		void AddCBVParameter(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility, uint32_t space = 0);

		/// <summary>
		/// srvを追加する
		/// </summary>
		/// <param name="shaderRegister">レジスタ番号</param>
		/// <param name="arryNum">配列の数</param>
		/// <param name="spaceNum">配置するスペース</param>
		/// <param name="visibility">使用するシェーダー</param>
		/// <param name="startNum">配列の開始位置</param>
		void AddSRVDescriptorTable(uint32_t shaderRegister, uint32_t arrayNum, uint32_t spaceNum, D3D12_SHADER_VISIBILITY visibility,
			uint32_t startNum = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

		/// <summary>
		/// UAVを追加する
		/// </summary>
		/// <param name="shaderRegister">レジスタ番号</param>
		/// <param name="arryNum">配列の数</param>
		/// <param name="spaceNum"></param>
		/// <param name="visibility">使用するシェーダー</param>
		/// <param name="startNum">配列の開始位置</param>
		void AddUAVDescriptorTable(uint32_t shaderRegister, uint32_t arrayNum, uint32_t spaceNum, D3D12_SHADER_VISIBILITY visibility,
			uint32_t startNum = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

		/// <summary>
		/// サンプラーを追加する
		/// </summary>
		/// <param name="shaderRegister">レジスタ番号</param>
		/// <param name="filter">バイナリフィルタ</param>
		/// <param name="texAddress">範囲外の振る舞い</param>
		/// <param name="visibility">使用するシェーダー</param>
		void AddSampler(uint32_t shaderRegister, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE texAddress, D3D12_SHADER_VISIBILITY visibility, D3D12_COMPARISON_FUNC func = D3D12_COMPARISON_FUNC_NEVER);

		/// <summary>
		/// 32bitルート定数を追加する
		/// </summary>
		/// <param name="num32BitValues">32bit値の個数</param>
		/// <param name="shaderRegister">レジスタ番号</param>
		/// <param name="visibility">使用するシェーダー</param>
		/// <param name="space">レジスタスペース</param>
		void Add32BitConstantsParameter(uint32_t num32BitValues, uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility, uint32_t space = 0);

		/// <summary>
		/// ルートシグネチャを手動生成
		/// </summary>
		/// <param name="flags"></param>
		void CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		/// <summary>
		/// shaderReflectionを使用して自動生成
		/// </summary>
		/// <param name="vsBlob"></param>
		/// <param name="psBlob"></param>
		void CreateRootSignatureFromReflection(IDxcUtils* utils,IDxcBlob* vsBlob, IDxcBlob* psBlob);

		ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
		std::vector<ParameterType> GetParameterTypes() const { return parameterTypes_; }

		Microsoft::WRL::ComPtr<ID3D12RootSignature> MoveOwnerRootSignature() { return std::move(rootSignature_); }

		// リセット
		void Reset();
	private:

		ID3D12Device* device_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

		std::vector<D3D12_ROOT_PARAMETER> rootParameters_;
		std::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges_;
		std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers_;
		
		// パラメータのタイプ
		std::vector<ParameterType> parameterTypes_;

		std::vector<PendingTable> pendingTables_;

	private:

		void AddDescriptorTable(ParameterType paramType,uint32_t shaderRegister, uint32_t startNum, uint32_t arrayNum, uint32_t spaceNum,
			D3D12_DESCRIPTOR_RANGE_TYPE rangeType, D3D12_SHADER_VISIBILITY visibility);

		void SerializeAndCreate(D3D12_ROOT_SIGNATURE_FLAGS flags);

		// hlslから使用するリソースを取得する
		void ReflectionBoundResourceToMap(IDxcUtils* utils, DxcBuffer reflectionBuffer, IDxcBlob* shaderBlob, D3D12_SHADER_VISIBILITY visibility,
			std::map<uint32_t, ResourceInfo>& cbvMap, std::map<uint32_t, ResourceInfo>& srvMap, std::map<uint32_t, ResourceInfo>& uavMap, std::map<uint32_t, ResourceInfo>& samplerMap);

		// リソースを適応させる
		void CreateParametersFromMaps(const std::map<uint32_t, ResourceInfo>& cbvMap, const std::map<uint32_t, ResourceInfo>& srvMap, const std::map<uint32_t, ResourceInfo>& uavMap, const std::map<uint32_t, ResourceInfo>& samplerMap);

		D3D12_SHADER_VISIBILITY MergeVisibility(D3D12_SHADER_VISIBILITY v1, D3D12_SHADER_VISIBILITY v2);

		std::string VisibilityToString(D3D12_SHADER_VISIBILITY v);
		std::string RangeTypeToString(D3D12_DESCRIPTOR_RANGE_TYPE t);
	};
}
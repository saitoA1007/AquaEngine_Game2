#include "pch.h"
#include "RootSignatureBuilder.h"
#include <cassert>
#include "LogManager.h"
#include <d3d12shader.h>

using namespace GameEngine;

void RootSignatureBuilder::Initialize(ID3D12Device* device) {
	device_ = device;
    descriptorRanges_.reserve(8);
}

void RootSignatureBuilder::Reset() {
    rootParameters_.clear();
    descriptorRanges_.clear();
    staticSamplers_.clear();
    parameterTypes_.clear();
    pendingTables_.clear();
    rootSignature_.Reset();
}

void RootSignatureBuilder::AddCBVParameter(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility, uint32_t space) {
	D3D12_ROOT_PARAMETER param = {};
	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	param.ShaderVisibility = visibility;
	param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = space;
	rootParameters_.push_back(param);
    parameterTypes_.push_back(ParameterType::CBV);
}

void RootSignatureBuilder::AddSRVDescriptorTable(uint32_t shaderRegister, uint32_t arrayNum, uint32_t spaceNum, 
    D3D12_SHADER_VISIBILITY visibility, uint32_t startNum) {
    AddDescriptorTable(ParameterType::SRV, shaderRegister, startNum, arrayNum, spaceNum, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, visibility);
}

void RootSignatureBuilder::AddUAVDescriptorTable(uint32_t shaderRegister, uint32_t arrayNum, uint32_t spaceNum,
    D3D12_SHADER_VISIBILITY visibility, uint32_t startNum) {
    AddDescriptorTable(ParameterType::UAV, shaderRegister, startNum, arrayNum, spaceNum, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, visibility);
}

void RootSignatureBuilder::AddSampler(uint32_t shaderRegister, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE texAddress, D3D12_SHADER_VISIBILITY visibility, D3D12_COMPARISON_FUNC func) {
    D3D12_STATIC_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = filter; // バイリニアフィルタ
    samplerDesc.AddressU = texAddress;
    samplerDesc.AddressV = texAddress;
    samplerDesc.AddressW = texAddress;
    samplerDesc.ComparisonFunc = func;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplerDesc.ShaderRegister = shaderRegister; // レジスタ番号
    samplerDesc.ShaderVisibility = visibility; // シェーダーモード
    staticSamplers_.push_back(samplerDesc);
}

void RootSignatureBuilder::Add32BitConstantsParameter(uint32_t num32BitValues, uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility, uint32_t space) {
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.ShaderVisibility = visibility;
    param.Constants.Num32BitValues = num32BitValues;
    param.Constants.ShaderRegister = shaderRegister;
    param.Constants.RegisterSpace = space;

    rootParameters_.push_back(param);
    parameterTypes_.push_back(ParameterType::RootConstant);
}

void RootSignatureBuilder::CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flags) {
    SerializeAndCreate(flags);
}

void RootSignatureBuilder::CreateRootSignatureFromReflection(IDxcUtils* utils,IDxcBlob* vsBlob, IDxcBlob* psBlob) {

	DxcBuffer reflectionBuffer{};
	reflectionBuffer.Encoding = DXC_CP_ACP;

	// 頂点シェーダーとピクセルシェーダーのリフレクションを取得
	IDxcBlob* shaderBlobs[2] = { vsBlob, psBlob };
	D3D12_SHADER_VISIBILITY visibilities[2] = {
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_SHADER_VISIBILITY_PIXEL
	};

    // 同じパラメータを統合するためのマップ
    std::map<uint32_t, ResourceInfo> cbvMap;
    std::map<uint32_t, ResourceInfo> srvMap;
    std::map<uint32_t, ResourceInfo> uavMap;
    std::map<uint32_t, ResourceInfo> samplerMap;

    // 分析する
    for (uint32_t index = 0; index < 2; ++index) {
        ReflectionBoundResourceToMap(utils, reflectionBuffer, shaderBlobs[index], visibilities[index], cbvMap, srvMap, uavMap, samplerMap);
    }

    // 取得したパラメータマップからルートパラメータを作成
    CreateParametersFromMaps(cbvMap, srvMap, uavMap, samplerMap);

    // 作成する
    SerializeAndCreate(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void RootSignatureBuilder::AddDescriptorTable(ParameterType paramType,uint32_t shaderRegister, uint32_t startNum, uint32_t arrayNum, uint32_t spaceNum,
    D3D12_DESCRIPTOR_RANGE_TYPE rangeType, D3D12_SHADER_VISIBILITY visibility) {

    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = rangeType; // 使用するタイプ
    range.NumDescriptors = arrayNum; // 数
    range.BaseShaderRegister = shaderRegister; // 使用するレジスタ番号
    range.RegisterSpace = spaceNum; // 使用するスペース
    range.OffsetInDescriptorsFromTableStart = startNum;
    descriptorRanges_.push_back(range);

    D3D12_ROOT_PARAMETER param{};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.ShaderVisibility = visibility; // 使用するシェーダー
    param.DescriptorTable.NumDescriptorRanges = 1;
    param.DescriptorTable.pDescriptorRanges = nullptr;
    rootParameters_.push_back(param);
    parameterTypes_.push_back(paramType);

    PendingTable pt{};
    pt.paramIndex = static_cast<UINT>(rootParameters_.size() - 1);
    pt.rangeIndex = static_cast<UINT>(descriptorRanges_.size() - 1);
    pendingTables_.push_back(pt);
}

void RootSignatureBuilder::SerializeAndCreate(D3D12_ROOT_SIGNATURE_FLAGS flags) {
    // ポインタを設定する
    for (const auto& pt : pendingTables_) {
        rootParameters_[pt.paramIndex].DescriptorTable.pDescriptorRanges = &descriptorRanges_[pt.rangeIndex];
    }

    // ルートシグネチャの作成
    D3D12_ROOT_SIGNATURE_DESC desc{};
    if (!rootParameters_.empty()) {
        desc.NumParameters = static_cast<UINT>(rootParameters_.size());
        desc.pParameters = rootParameters_.data();
    }
    if (!staticSamplers_.empty()) {
        desc.NumStaticSamplers = static_cast<UINT>(staticSamplers_.size());
        desc.pStaticSamplers = staticSamplers_.data();
    }
    desc.Flags = flags;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            LogManager::GetInstance().Log(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
        }
        assert(false && "D3D12SerializeRootSignature failed");
        return;
    }
    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature_.ReleaseAndGetAddressOf()));
    assert(SUCCEEDED(hr) && "CreateRootSignature failed");
}

void RootSignatureBuilder::ReflectionBoundResourceToMap(IDxcUtils* utils, DxcBuffer reflectionBuffer, IDxcBlob* shaderBlob, D3D12_SHADER_VISIBILITY visibility,
    std::map<uint32_t, ResourceInfo>& cbvMap, std::map<uint32_t, ResourceInfo>& srvMap,
    std::map<uint32_t, ResourceInfo>& uavMap, std::map<uint32_t, ResourceInfo>& samplerMap) {

    reflectionBuffer.Ptr = shaderBlob->GetBufferPointer();
    reflectionBuffer.Size = shaderBlob->GetBufferSize();

    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
    HRESULT hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection));
    if (FAILED(hr)) {
        assert(0);
    }

    D3D12_SHADER_DESC shaderDesc{};
    reflection->GetDesc(&shaderDesc);

    auto insertOrMerge = [&](
        std::map<uint32_t, ResourceInfo>& map,
        const D3D12_SHADER_INPUT_BIND_DESC& bind,
        uint32_t bindCount = 1)
        {
            auto it = map.find(bind.BindPoint);
            if (it != map.end()) {
                // 同じレジスタが複数シェーダーで使われる場合は visibility を統合する
                it->second.visibility = MergeVisibility(it->second.visibility, visibility);
            } else {
                ResourceInfo info{};
                info.visibility = visibility;
                info.bindPoint = bind.BindPoint;
                info.space = bind.Space;
                info.bindCount = bindCount;
                map[bind.BindPoint] = info;
            }
        };

    // 解析したデータをマップに保存する
    for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
        reflection->GetResourceBindingDesc(i, &bindDesc);

        switch (bindDesc.Type) {
        case D3D_SIT_CBUFFER: {
            insertOrMerge(cbvMap, bindDesc, 1);
            break;
        }

        case D3D_SIT_TEXTURE:
        case D3D_SIT_STRUCTURED:
        case D3D_SIT_BYTEADDRESS: {
            insertOrMerge(srvMap, bindDesc, bindDesc.BindCount);
            break;
        }

        case D3D_SIT_UAV_RWTYPED:
        case D3D_SIT_UAV_RWSTRUCTURED:
        case D3D_SIT_UAV_RWBYTEADDRESS: {
            insertOrMerge(uavMap, bindDesc, bindDesc.BindCount);
            break;
        }

        case D3D_SIT_SAMPLER: {
            insertOrMerge(samplerMap, bindDesc, 1);
            break;
        }
        }
    }
}

void RootSignatureBuilder::CreateParametersFromMaps(const std::map<uint32_t, ResourceInfo>& cbvMap, const std::map<uint32_t, ResourceInfo>& srvMap,
    const std::map<uint32_t, ResourceInfo>& uavMap, const std::map<uint32_t, ResourceInfo>& samplerMap){
    // CBVを追加
    for (const auto& [key, info] : cbvMap) {
        AddCBVParameter(info.bindPoint, info.visibility, info.space);
        LogManager::GetInstance().Log("arrayNum" + std::to_string(rootParameters_.size() - 1) + " / Type : CBV / registerNum : " + std::to_string(info.bindPoint) + " / visibility : " + VisibilityToString(info.visibility));
    }

    // SRVを追加
    for (const auto& [key, info] : srvMap) {
        AddSRVDescriptorTable(info.bindPoint, info.bindCount, info.space, info.visibility);
        LogManager::GetInstance().Log("arrayNum" + std::to_string(rootParameters_.size() - 1) + " / Type : SRV / registerNum : " + std::to_string(info.bindPoint) + " / visibility : " + VisibilityToString(info.visibility));
    }

    // UAV
    for (const auto& [key, info] : uavMap) {
        AddUAVDescriptorTable(info.bindPoint, info.bindCount, info.space, info.visibility);
        LogManager::GetInstance().Log("arrayNum" + std::to_string(rootParameters_.size() - 1) + " / Type : UAV / registerNum : " + std::to_string(info.bindPoint) + " / visibility : " + VisibilityToString(info.visibility));
    }

    // サンプラーを追加
    for (const auto& [key, info] : samplerMap) {
        AddSampler(info.bindPoint, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, info.visibility);
    }
}

D3D12_SHADER_VISIBILITY RootSignatureBuilder::MergeVisibility(D3D12_SHADER_VISIBILITY v1, D3D12_SHADER_VISIBILITY v2) {
    // 既にAllならそのまま返す
    if (v1 == D3D12_SHADER_VISIBILITY_ALL || v2 == D3D12_SHADER_VISIBILITY_ALL) {
        return D3D12_SHADER_VISIBILITY_ALL;
    }

    // 異なる形で使用されている場合はAll
    if (v1 != v2) {
        return D3D12_SHADER_VISIBILITY_ALL;
    }

    // 同じならそのまま返す
    return v1;
}

std::string RootSignatureBuilder::VisibilityToString(D3D12_SHADER_VISIBILITY v) {
    switch (v) {
    case D3D12_SHADER_VISIBILITY_ALL:      return "ALL";
    case D3D12_SHADER_VISIBILITY_VERTEX:   return "VS";
    case D3D12_SHADER_VISIBILITY_HULL:     return "HS";
    case D3D12_SHADER_VISIBILITY_DOMAIN:   return "DS";
    case D3D12_SHADER_VISIBILITY_GEOMETRY: return "GS";
    case D3D12_SHADER_VISIBILITY_PIXEL:    return "PS";
    default:                               return "UNKNOWN";
    }
}

std::string RootSignatureBuilder::RangeTypeToString(D3D12_DESCRIPTOR_RANGE_TYPE t) {
    switch (t) {
    case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:     return "SRV";
    case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:     return "UAV";
    case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:     return "CBV";
    case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER: return "Sampler";
    default:                                  return "UNKNOWN";
    }
}
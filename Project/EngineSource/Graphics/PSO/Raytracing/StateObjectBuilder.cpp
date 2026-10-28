#include "pch.h"
#include "StateObjectBuilder.h"
#include <cassert>

using namespace GameEngine;

void StateObjectBuilder::Initialize(D3D12_STATE_OBJECT_TYPE type) {
    exportAssociationStorage_.clear();

    desc_.SetStateObjectType(type);
}

void StateObjectBuilder::AddDXILLibrary(IDxcBlob* blob, const std::vector<std::wstring>& exportNames) {
    auto dxil = desc_.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

    D3D12_SHADER_BYTECODE libdxil{};
    libdxil.pShaderBytecode = blob->GetBufferPointer();
    libdxil.BytecodeLength = blob->GetBufferSize();
    dxil->SetDXILLibrary(&libdxil);

    for (const auto& name : exportNames) {
        dxil->DefineExport(name.c_str());
    }
}

void StateObjectBuilder::AddHitGroup(
    const std::wstring& hitGroupName,
    const std::wstring& closestHitShaderName,
    const std::wstring& anyHitShaderName,
    const std::wstring& intersectionShaderName,
    D3D12_HIT_GROUP_TYPE type) {
    auto hg = desc_.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hg->SetHitGroupExport(hitGroupName.c_str());
    hg->SetHitGroupType(type);

    if (!closestHitShaderName.empty()) { 
        hg->SetClosestHitShaderImport(closestHitShaderName.c_str()); 
    }
    if (!anyHitShaderName.empty()) {
        hg->SetAnyHitShaderImport(anyHitShaderName.c_str()); 
    }
    if (!intersectionShaderName.empty()) {
        hg->SetIntersectionShaderImport(intersectionShaderName.c_str()); 
    }
}

void StateObjectBuilder::SetShaderConfig(const UINT& maxPayloadSize, const UINT& maxAttributeSize) {
    auto config = desc_.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    config->Config(maxPayloadSize, maxAttributeSize);
}

void StateObjectBuilder::SetPipelineConfig(const UINT& maxRecursionDepth) {
    auto config = desc_.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    config->Config(maxRecursionDepth);
}

void StateObjectBuilder::SetGlobalRootSignature(ID3D12RootSignature* rootSignature) {
    auto rs = desc_.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    rs->SetRootSignature(rootSignature);
}

void StateObjectBuilder::AddLocalRootSignature(
    ID3D12RootSignature* rootSignature,
    const std::vector<std::wstring>& shaderNames) {
    auto lrs = desc_.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    lrs->SetRootSignature(rootSignature);
    auto assoc = desc_.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    assoc->SetSubobjectToAssociate(*lrs);

    // LPCWSTRの配列を寿命管理リストに保持する
    std::vector<LPCWSTR> ptrs;
    ptrs.reserve(shaderNames.size());
    for (const auto& name : shaderNames) {
        assoc->AddExport(name.c_str());
    }
}

Microsoft::WRL::ComPtr<ID3D12StateObject> StateObjectBuilder::Build(ID3D12Device5* device) {
    Microsoft::WRL::ComPtr<ID3D12StateObject> stateObject;
    HRESULT hr = device->CreateStateObject(desc_, IID_PPV_ARGS(&stateObject));
    assert(SUCCEEDED(hr) && "CreateStateObject failed");
    return stateObject;
}
#include "pch.h"
#include"InputLayoutBuilder.h"
#include <d3d12shader.h>
#include<cassert>
#include "LogManager.h"
using namespace GameEngine;

void InputLayoutBuilder::CreateInputElement(const std::string& name, uint32_t index, uint32_t slotIndex, DXGI_FORMAT format) {
	// InputLayout
    semanticNames_.push_back(name);
	D3D12_INPUT_ELEMENT_DESC inputElementDescs = {};
	inputElementDescs.SemanticName = nullptr;
	inputElementDescs.SemanticIndex = index;
	inputElementDescs.InputSlot = slotIndex;
	inputElementDescs.Format = format;
	inputElementDescs.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	// 格納する
	inputElementDescs_.push_back(std::move(inputElementDescs));
}

void InputLayoutBuilder::CreateInputLayoutDesc() {
	inputLayoutDesc_.pInputElementDescs = inputElementDescs_.data();
	inputLayoutDesc_.NumElements = static_cast<UINT>(inputElementDescs_.size());
}

void InputLayoutBuilder::Reset() {
	inputElementDescs_.clear();
    semanticNames_.clear();
}

void InputLayoutBuilder::CreateDefaultObjElement() {
    Reset();
	CreateInputElement("POSITION", 0,0, DXGI_FORMAT_R32G32B32A32_FLOAT);
	CreateInputElement("TEXCOORD", 0, 0, DXGI_FORMAT_R32G32_FLOAT);
	CreateInputElement("NORMAL", 0, 0, DXGI_FORMAT_R32G32B32_FLOAT);
    CreateInputElement("TANGENT", 0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT);
    SetSemanticName();
	CreateInputLayoutDesc();
}

void InputLayoutBuilder::CreateDefaultSpriteElement() {
    Reset();
	CreateInputElement("POSITION", 0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT);
	CreateInputElement("TEXCOORD", 0, 0, DXGI_FORMAT_R32G32_FLOAT);
    SetSemanticName();
	CreateInputLayoutDesc();
}

void InputLayoutBuilder::CreateDefaultLineElement() {
    Reset();
	CreateInputElement("POSITION", 0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT);
	CreateInputElement("COLOR", 0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT);
    SetSemanticName();
	CreateInputLayoutDesc();
}

void InputLayoutBuilder::CreateDefaultAnimationElement() {
    Reset();
	CreateInputElement("POSITION", 0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT);
	CreateInputElement("TEXCOORD", 0, 0, DXGI_FORMAT_R32G32_FLOAT);
	CreateInputElement("NORMAL", 0, 0, DXGI_FORMAT_R32G32B32_FLOAT);
    CreateInputElement("TANGENT", 0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT);
	CreateInputElement("WEIGHT", 0, 1, DXGI_FORMAT_R32G32B32A32_FLOAT);
	CreateInputElement("INDEX", 0, 1, DXGI_FORMAT_R32G32B32A32_SINT);
    SetSemanticName();
	CreateInputLayoutDesc();
}

void InputLayoutBuilder::CreateGridElement() {
    Reset();
    CreateInputElement("POSITION", 0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT);
    SetSemanticName();
    CreateInputLayoutDesc();
}

void InputLayoutBuilder::CreateNone() {
    inputLayoutDesc_.pInputElementDescs = nullptr;
    inputLayoutDesc_.NumElements = 0;
}

void InputLayoutBuilder::CreateInputLayoutFromReflection(IDxcUtils* utils,IDxcBlob* vsBlob) {
    DxcBuffer reflectionBuffer{};
    reflectionBuffer.Ptr = vsBlob->GetBufferPointer();
    reflectionBuffer.Size = vsBlob->GetBufferSize();
    reflectionBuffer.Encoding = DXC_CP_ACP;

    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
    HRESULT hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection));
    assert(SUCCEEDED(hr));

    D3D12_SHADER_DESC shaderDesc{};
    reflection->GetDesc(&shaderDesc);

    inputElementDescs_.clear();
    semanticNames_.clear();
    // メモリを事前に確保
    semanticNames_.reserve(shaderDesc.InputParameters);

    // 入力パラメータを解析
    for (UINT i = 0; i < shaderDesc.InputParameters; ++i) {
        D3D12_SIGNATURE_PARAMETER_DESC paramDesc{};
        reflection->GetInputParameterDesc(i, &paramDesc);

        D3D12_INPUT_ELEMENT_DESC elementDesc{};

        // セマンティック名を保存
        semanticNames_.push_back(paramDesc.SemanticName);
        elementDesc.SemanticName = semanticNames_.back().data();
        elementDesc.SemanticIndex = paramDesc.SemanticIndex;
        elementDesc.InputSlot = 0; // デフォルトはスロット0
        elementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

        // コンポーネントタイプとマスクからフォーマットを決定
        if (paramDesc.Mask == 1) {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) { 
                elementDesc.Format = DXGI_FORMAT_R32_UINT;
            } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) { 
                elementDesc.Format = DXGI_FORMAT_R32_SINT;
            } else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
                elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
            }
        } else if (paramDesc.Mask <= 3) {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) { 
                elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
            }
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) { 
                elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
            }
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) { 
                elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT; 
            }
        } else if (paramDesc.Mask <= 7) {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
                elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT; 
            }
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) { 
                elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
            }
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
                elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
            }
        } else if (paramDesc.Mask <= 15) {
            if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            }
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) { 
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            }
            else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
                elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }

        inputElementDescs_.push_back(elementDesc);
    }

    // 入力レイアウトDescを作成
    CreateInputLayoutDesc();
}

void InputLayoutBuilder::SetSemanticName() {
    // 要素数が一致しているか確認
    assert(inputElementDescs_.size() == semanticNames_.size());
    // 名前を適応させる
    for (size_t i = 0; i < inputElementDescs_.size(); ++i) {
        inputElementDescs_[i].SemanticName = semanticNames_[i].c_str();
    }
}
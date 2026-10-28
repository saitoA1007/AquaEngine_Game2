#include "pch.h"
#include "RayLibShaderCompiler.h"
#include <cassert>
#include <d3dcompiler.h>

namespace fs = std::filesystem;
using namespace GameEngine;

void RayLibShaderCompiler::Initialize(DXC* dxc) {
	dxc_ = dxc;
    fs::create_directories(csoDirectory_);
}

LibraryResult RayLibShaderCompiler::CompileShader(const std::wstring& hlslPath) {

    LibraryResult result;
    const std::wstring csoPath = GetCsoPath(hlslPath);

    // CSOがあり、HLSLより新しければ読み込む
    if (fs::exists(csoPath) && fs::last_write_time(hlslPath) <= fs::last_write_time(csoPath)) {
        result.blob = LoadCso(csoPath);
    }
    // なければ、または読み込みに失敗したらコンパイルして保存
    if (!result.blob) {
        result.blob = dxc_->CompileShader(hlslPath, kLibProfile_.c_str(), L"");
        SaveCso(csoPath, result.blob.Get());
    }

    result.exportNames = ReflectExportNames(result.blob.Get());
    return result;
}

std::vector<std::wstring> RayLibShaderCompiler::ReflectExportNames(IDxcBlob* blob) {
    std::vector<std::wstring> names;

    // コンテナリフレクションを作成
    Microsoft::WRL::ComPtr<IDxcContainerReflection> containerReflection;
    HRESULT hr = DxcCreateInstance(
        CLSID_DxcContainerReflection,
        IID_PPV_ARGS(&containerReflection)
    );
    assert(SUCCEEDED(hr));

    hr = containerReflection->Load(blob);
    assert(SUCCEEDED(hr));

    // DXILパートを探す
    UINT32 partIndex = 0;
    hr = containerReflection->FindFirstPartKind(
        DXC_PART_DXIL, // 0x4C495844
        &partIndex
    );
    assert(SUCCEEDED(hr));

    // ID3D12LibraryReflectionを取得
    Microsoft::WRL::ComPtr<ID3D12LibraryReflection> libraryReflection;
    hr = containerReflection->GetPartReflection(
        partIndex,
        IID_PPV_ARGS(&libraryReflection)
    );
    assert(SUCCEEDED(hr));

    // エクスポート関数名を列挙
    D3D12_LIBRARY_DESC libDesc{};
    libraryReflection->GetDesc(&libDesc);

    for (UINT i = 0; i < libDesc.FunctionCount; ++i) {
        auto* func = libraryReflection->GetFunctionByIndex(i);
        D3D12_FUNCTION_DESC funcDesc{};
        func->GetDesc(&funcDesc);

        std::string rawName(funcDesc.Name);
        // 先頭の\x1?を取り除く
        if (rawName.size() > 2 && rawName[0] == '\x1' && rawName[1] == '?') {
            rawName = rawName.substr(2);
            // @@以降のシグネチャ部分を除去
            auto at = rawName.find("@@");
            if (at != std::string::npos) {
                rawName = rawName.substr(0, at);
            }
        }
        names.push_back(std::wstring(rawName.begin(), rawName.end()));
    }

    return names;
}

std::wstring RayLibShaderCompiler::GetCsoPath(const std::wstring& hlslPath) const {
    return csoDirectory_ + fs::path(hlslPath).stem().wstring() + L"_lib.cso";
}

Microsoft::WRL::ComPtr<IDxcBlob> RayLibShaderCompiler::LoadCso(const std::wstring& csoPath) {
    std::ifstream file(csoPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) { return nullptr; }
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) { return nullptr; }

    Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob;
    if (FAILED(dxc_->dxcUtils_->CreateBlob(buffer.data(), static_cast<UINT32>(size), CP_ACP, &blob))) {
        return nullptr;
    }
    return blob;
}

void RayLibShaderCompiler::SaveCso(const std::wstring& csoPath, IDxcBlob* blob) {
    std::ofstream file(csoPath, std::ios::binary);
    if (!file.is_open()) { return; }
    file.write(static_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());
}
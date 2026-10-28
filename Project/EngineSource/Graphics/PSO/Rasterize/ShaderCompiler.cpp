#include "pch.h"
#include "ShaderCompiler.h"
#include <cassert>
#include <fstream>
#include <filesystem>

#ifdef USE_IMGUI
#include "NodeSystem/MaterialShaderGenerator.h"
#endif

namespace fs = std::filesystem;
using namespace GameEngine;

void ShaderCompiler::Initialize(DXC* dxc) {
	dxc_ = dxc;

	// CSOディレクトリが存在しない場合は作成
	fs::path csoDir(csoDirectory_);
	if (!fs::exists(csoDir)) {
		fs::create_directories(csoDir);
	}

	// マテリアルグラフから生成されるHLSLの保存先が存在しない場合は作成
	fs::path genDir(generatedHlslDirectory_);
	if (!fs::exists(genDir)) {
		fs::create_directories(genDir);
	}
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::CompileShader(Type type, const std::wstring& path) {
	std::wstring csoPath = GetCsoPath(path);

#ifdef USE_IMGUI
	// Debug版ではHLSLが更新されていればコンパイル、なければCSOを読み込む
	if (!fs::exists(csoPath) || IsHlslNewer(path, csoPath)) {
		// コンパイルして保存
		return CompileAndSave(type, path);
	} else {
		// 既存のCSOを読み込み
		return LoadCsoFile(csoPath);
	}
#else
	// Release版ではCSOファイルを読み込む
	Microsoft::WRL::ComPtr<IDxcBlob> blob = LoadCsoFile(csoPath);
	assert(blob != nullptr && "CSO file not found in Release build");
	return blob;
#endif
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::CompileMaterialGraph(const MaterialGraph& graph, const std::wstring& materialName) {
#ifdef USE_IMGUI
	// グラフからHLSLソースを生成
	std::string hlslSource = MaterialShaderGenerator::Generate(graph);
	assert(!hlslSource.empty() && "MaterialGraph: HLSL生成に失敗しました");

	// 生成先のパスを決定し、ファイルを生成
	std::wstring hlslPath = generatedHlslDirectory_ + materialName + L".PS.hlsl";
	WriteGeneratedHlsl(hlslPath, hlslSource);

	// コンパイルする
	return CompileShader(Type::PS, hlslPath);
#else
	return nullptr;
#endif
}

std::wstring ShaderCompiler::GetCsoPath(const std::wstring& hlslPath) {
	// パスからファイル名を取得
	fs::path path(hlslPath);
	std::wstring filename = path.filename().wstring();

	// 拡張子を.csoに変更
	size_t dotPos = filename.find_last_of(L'.');
	if (dotPos != std::wstring::npos) {
		filename = filename.substr(0, dotPos) + L".cso";
	} else {
		filename += L".cso";
	}

	// CSOディレクトリのパスと結合
	return csoDirectory_ + filename;
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::LoadCsoFile(const std::wstring& csoPath) {
	// ファイルを開く
	std::ifstream file(csoPath, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		return nullptr;
	}

	// ファイルサイズを取得
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	// データを読み込む
	std::vector<char> buffer(size);
	if (!file.read(buffer.data(), size)) {
		return nullptr;
	}
	file.close();

	// IDxcBlobを作成
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob;
	HRESULT hr = dxc_->dxcUtils_->CreateBlob(
		buffer.data(),
		static_cast<UINT32>(size),
		CP_ACP,
		&blob
	);

	if (FAILED(hr)) {
		return nullptr;
	}

	return blob;
}

void ShaderCompiler::SaveCsoFile(const std::wstring& csoPath, IDxcBlob* blob) {
	// ファイルに書き込む
	std::ofstream file(csoPath, std::ios::binary);
	if (!file.is_open()) {
		assert(false && "Failed to open CSO file for writing");
		return;
	}
	file.write(static_cast<const char*>(blob->GetBufferPointer()),blob->GetBufferSize());
	file.close();
}

bool ShaderCompiler::IsHlslNewer(const std::wstring& hlslPath, const std::wstring& csoPath) {
	fs::path hlsl(hlslPath);
	fs::path cso(csoPath);

	if (!fs::exists(hlsl) || !fs::exists(cso)) {
		return true;
	}

	// 更新日時を比較
	auto hlslTime = fs::last_write_time(hlsl);
	auto csoTime = fs::last_write_time(cso);

	return hlslTime > csoTime;
}

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::CompileAndSave(Type type, const std::wstring& hlslPath) {
	// HLSLをコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = dxc_->CompileShader(hlslPath,compileTypes[static_cast<size_t>(type)].c_str());

	assert(shaderBlob != nullptr && "Shader compilation failed");

	// CSOファイルとして保存
	std::wstring csoPath = GetCsoPath(hlslPath);
	SaveCsoFile(csoPath, shaderBlob.Get());
	return shaderBlob;
}

void ShaderCompiler::WriteGeneratedHlsl(const std::wstring& hlslPath, const std::string& source) {
	// 内容が変わっていなければタイムスタンプを更新しない
	std::ifstream existing(hlslPath, std::ios::binary);
	if (existing.is_open()) {
		std::string existingContent(
			(std::istreambuf_iterator<char>(existing)),
			std::istreambuf_iterator<char>());
		if (existingContent == source) {
			return;
		}
	}
	existing.close();

	std::ofstream out(hlslPath, std::ios::binary);
	assert(out.is_open() && "Failed to write generated material HLSL");
	out << source;
}
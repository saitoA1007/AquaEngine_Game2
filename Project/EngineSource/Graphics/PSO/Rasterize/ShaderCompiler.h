#pragma once
#include "DXC.h"
#include <string>
#include <filesystem>
#include "NodeSystem/MaterialGraph.h"

namespace GameEngine {

	class ShaderCompiler {
	public:

		// コンパイルするシェーダータイプ
		enum class Type {
			VS,
			PS,
			Cs,

			Count
		};

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="dxc"></param>
		void Initialize(DXC* dxc);

		// コンパイルするシェーダー
		Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(Type type, const std::wstring& path);

		// マテリアルグラフからhlslを生成してコンパイルする
		Microsoft::WRL::ComPtr<IDxcBlob> CompileMaterialGraph(const MaterialGraph& graph, const std::wstring& materialName);

	private:

		DXC* dxc_ = nullptr;

		// コンパイルタイプ
		std::wstring compileTypes[static_cast<size_t>(Type::Count)] = {
			L"vs_6_5",
			L"ps_6_5",
			L"cs_6_5"
		};

		// csoファイルのディレクトリパス
#ifdef _DEBUG
		const std::wstring csoDirectory_ = L"Resources/Shaders/Compiled/Debug/";
#else
		const std::wstring csoDirectory_ = L"Resources/Shaders/Compiled/Release/";
#endif

		// 生成するファイルパス
		const std::wstring generatedHlslDirectory_ = L"Resources/Shaders/Material/Generated/";

	private:

		/// <summary>
		/// HLSLからCSOを生成
		/// </summary>
		std::wstring GetCsoPath(const std::wstring& hlslPath);

		/// <summary>
		/// CSOファイルを読み込む
		/// </summary>
		Microsoft::WRL::ComPtr<IDxcBlob> LoadCsoFile(const std::wstring& csoPath);

		/// <summary>
		/// CSOファイルに保存
		/// </summary>
		void SaveCsoFile(const std::wstring& csoPath, IDxcBlob* blob);

		/// <summary>
		/// HLSLファイルがCSOより新しいかチェック
		/// </summary>
		bool IsHlslNewer(const std::wstring& hlslPath, const std::wstring& csoPath);

		/// <summary>
		/// HLSLをコンパイルしてCSOとして保存
		/// </summary>
		Microsoft::WRL::ComPtr<IDxcBlob> CompileAndSave(Type type, const std::wstring& hlslPath);

		/// <summary>
		/// 生成したHLSLをファイルに書き出す
		/// </summary>
		/// <param name="hlslPath"></param>
		/// <param name="source"></param>
		void WriteGeneratedHlsl(const std::wstring& hlslPath, const std::string& source);
	};
}
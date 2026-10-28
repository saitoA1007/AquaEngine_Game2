#pragma once
#include <vector>
#include "DXC.h"

namespace GameEngine {

	struct LibraryResult {
		Microsoft::WRL::ComPtr<IDxcBlob> blob;
		std::vector<std::wstring> exportNames;
	};

	class RayLibShaderCompiler {
	public:

		void Initialize(DXC* dxc);

		// シェーダーをコンパイルする
		LibraryResult CompileShader(const std::wstring& hlslPath);

	private:
		DXC* dxc_ = nullptr;

		const std::wstring kLibProfile_ = L"lib_6_6";

#ifdef _DEBUG
		const std::wstring csoDirectory_ = L"Resources/Shaders/Compiled/Debug/";
#else
		const std::wstring csoDirectory_ = L"Resources/Shaders/Compiled/Release/";
#endif

	private:

		// コンパイル済みBlobからエクスポート名を取得
		std::vector<std::wstring> ReflectExportNames(IDxcBlob* blob);

		std::wstring GetCsoPath(const std::wstring& hlslPath) const;

		Microsoft::WRL::ComPtr<IDxcBlob> LoadCso(const std::wstring& csoPath);

		void SaveCso(const std::wstring& csoPath, IDxcBlob* blob);
	};
}
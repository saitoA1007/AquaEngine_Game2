#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <string>
#include <cstdint>

#if defined(_DEBUG) || defined(USE_PIX)
#define ENGINE_USE_DEBUG_NAME 1
#else
#define ENGINE_USE_DEBUG_NAME 0
#endif

namespace GameEngine {

	/// <summary>
	/// D3D12オブジェクトにデバッグ名を設定する
	/// </summary>
	inline void SetDebugName(ID3D12Object* object, const std::string& name) {
#if ENGINE_USE_DEBUG_NAME
		if (object == nullptr) { return; }

		const int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, name.c_str(), static_cast<int>(name.size()), nullptr, 0);
		if (sizeNeeded <= 0) { return; }

		std::wstring wide(static_cast<size_t>(sizeNeeded), 0);
		MultiByteToWideChar(CP_UTF8, 0, name.c_str(), static_cast<int>(name.size()), wide.data(), sizeNeeded);

		object->SetName(wide.c_str());
#else
		(void)object; (void)name;
#endif
	}

	/// <summary>
	/// インデックス付きのデバッグ名を設定する
	/// </summary>
	inline void SetDebugName(ID3D12Object* object, const std::string& name, uint32_t index) {
#if ENGINE_USE_DEBUG_NAME
		SetDebugName(object, name + "[" + std::to_string(index) + "]");
#else
		(void)object; (void)name; (void)index;
#endif
	}
}

// リソース変数名をそのままデバッグ名にしたい場合用
#define SET_DEBUG_NAME_AUTO(object) ::GameEngine::SetDebugName((object), #object)

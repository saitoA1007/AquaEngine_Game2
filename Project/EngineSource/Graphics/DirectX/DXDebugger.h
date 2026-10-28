#pragma once
#ifdef _DEBUG
#include <d3d12.h>
#include <wrl.h>

class DXDebugger final {
public:

	void InitializeDebugLayer(ID3D12Device* device);
};
#endif


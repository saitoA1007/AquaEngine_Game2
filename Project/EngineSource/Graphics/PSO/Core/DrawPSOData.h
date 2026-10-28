#pragma once
#include <d3d12.h>

// 描画する際に必要なデータを送る
struct DrawPsoData {
	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* graphicsPipelineState;
};
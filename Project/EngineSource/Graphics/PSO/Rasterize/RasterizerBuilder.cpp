#include "pch.h"
#include"RasterizerBuilder.h"

using namespace GameEngine;

void RasterizerBuilder::Initialize() {
	for (uint32_t i = 0; i < DrawModel::kCountOfDrawMode; ++i) {

		switch (i) {
		case DrawModel::FillFront:
			// 裏面(時計回り)を表示しない
			rasterizerDesc[i].CullMode = D3D12_CULL_MODE_BACK;
			// 三角形の中を塗りつぶす
			rasterizerDesc[i].FillMode = D3D12_FILL_MODE_SOLID;
			break;

		case DrawModel::FrameFront:
			// 裏面(時計回り)を表示しない
			rasterizerDesc[i].CullMode = D3D12_CULL_MODE_BACK;
			// 三角形の中を塗りつぶす
			rasterizerDesc[i].FillMode = D3D12_FILL_MODE_WIREFRAME;
			break;

		case DrawModel::FrameBack:
			// 表(反時計回り)を表示しない
			rasterizerDesc[i].CullMode = D3D12_CULL_MODE_FRONT;
			// 三角形の中を塗りつぶす
			rasterizerDesc[i].FillMode = D3D12_FILL_MODE_WIREFRAME;
			break;

		case DrawModel::None:
			// 両面表示
			rasterizerDesc[i].CullMode = D3D12_CULL_MODE_NONE;
			// 三角形の中を塗りつぶす
			rasterizerDesc[i].FillMode = D3D12_FILL_MODE_SOLID;
			break;
		}
	}
}

D3D12_RASTERIZER_DESC RasterizerBuilder::GetRasterizerDesc(DrawModel drawMode) {
	return rasterizerDesc[drawMode];
}
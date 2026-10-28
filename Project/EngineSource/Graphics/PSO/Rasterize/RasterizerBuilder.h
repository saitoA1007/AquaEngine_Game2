#pragma once
#include<d3d12.h>
#include<dxcapi.h>
#include<stdint.h>

#include"BlendMode.h"

namespace GameEngine {

	class RasterizerBuilder {
	public:

		/// <summary>
		/// 各描画モードを作成
		/// </summary>
		void Initialize();

		D3D12_RASTERIZER_DESC GetRasterizerDesc(DrawModel drawMode);

	private:

		D3D12_RASTERIZER_DESC rasterizerDesc[DrawModel::kCountOfDrawMode]{};
	};
}


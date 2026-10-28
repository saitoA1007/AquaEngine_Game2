#pragma once
#include "GpuResource.h"
#include <cstdint>

namespace GameEngine {

	class BLAS : public GpuResource {
	public:
		BLAS() = default;
		~BLAS() = default;

		/// <summary>
		/// BLASを作成する
		/// </summary>
		void Create(ID3D12GraphicsCommandList4* cmdList,
			const D3D12_VERTEX_BUFFER_VIEW& vertexBufView, const D3D12_INDEX_BUFFER_VIEW& indexBufView,
			const uint32_t& totalVertices, const uint32_t& totalIndices, const bool& isUpdate);

		/// <summary>
		/// 更新処理
		/// </summary>
		/// <param name="cmdList"></param>
		void Update(ID3D12GraphicsCommandList4* cmdList, const D3D12_VERTEX_BUFFER_VIEW& vertexBufView);

	private:
		// ジオメトリ情報
		D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc_{};
		// 入力設定
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs_{};
		// 構築用バッファ
		Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer_;
	};
}
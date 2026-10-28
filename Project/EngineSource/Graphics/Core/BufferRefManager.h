#pragma once
#include <deque>
#include "StructuredBuffer.h"
#include "DescriptorCounts.h"

namespace GameEngine {

	// バッファ情報へのアクセスデータ
	struct BufferRef 
	{
		uint32_t type = 0;  // データのタイプ
		uint32_t materialIndex = 0; // マテリアルデータの参照するハンドル

		uint32_t indexHandle; // インデックス
		uint32_t vertexHandle; // 頂点

		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;
		float pad[2];
	};

	// バッファタイプ
	enum class BufferType {
		kDefalutMaterial,
		kDefalutMaterialTranslucent,

		kMaxCount
	};

	class BufferRefManager {
	public:
		BufferRefManager() = default;
		~BufferRefManager() = default;

		void Initialize();

		/// <summary>
		/// 空きインデックスを確保して返す
		/// </summary>
		/// <returns></returns>
		uint32_t AllocateIndex();

		/// <summary>
		/// インデックスを削除
		/// </summary>
		/// <param name="index"></param>
		void ReleaseIndex(const uint32_t& index);

		/// <summary>
		/// アクセスデータを取得
		/// </summary>
		/// <param name="index"></param>
		/// <returns></returns>
		BufferRef* GetBufferRef(const uint32_t& index);

		// bufferRefのGPUハンドルを取得
		const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetSrvHandleGPU() const { return bufferRefs_.GetSrvGpuHandle(); }
	private:
		BufferRefManager(const BufferRefManager&) = delete;
		BufferRefManager& operator=(const BufferRefManager&) = delete;

		// バッファにアクセスするためのデータ
		StructuredBuffer<BufferRef> bufferRefs_;

		// 最大数
		uint32_t maxCount_ = static_cast<uint32_t>(SrvHeapTypeCount::BufferMaxCount);
		// 次のインデックス
		uint32_t nextIndex_ = 0;

		// 解放されたインデックスのリスト
		std::deque<uint32_t> freeIndices_;
	};
}
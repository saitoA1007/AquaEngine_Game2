#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <queue>
#include <unordered_map>

#include "DescriptorCounts.h"

namespace GameEngine {

	class SrvManager {
	public:
		SrvManager() = default;
		~SrvManager() = default;

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="device"></param>
		void Initialize(ID3D12Device* device);

		ID3D12DescriptorHeap* GetSRVHeap() const { return srvHeap_.Get(); }

		/// <summary>
		/// インデックスを取得
		/// </summary>
		/// <returns></returns>
		uint32_t AllocateSrvIndex(SrvHeapType srvHeapType);

		/// <summary>
		/// インデックスを削除
		/// </summary>
		/// <param name="index"></param>
		void ReleaseIndex(uint32_t index);

		/// <summary>
		/// 指定インデックスのCPUディスクリプタハンドルを取得
		/// </summary>
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;

		/// <summary>
		/// 指定インデックスのGPUディスクリプタハンドルを取得
		/// </summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const;

		// heapのsrvIndexのスタート位置を取得
		uint32_t GetStartSrvIndex(SrvHeapType type);

	private:

		// メモリの領域データ
		struct HeapRange {
			uint32_t start;     // 開始インデックス
			uint32_t end;       // 終了インデックス
			uint32_t current;   // 次に割り当てる新規インデックス
			std::queue<uint32_t> freeList; // 解放されたインデックス
		};

	private:
		SrvManager(const SrvManager&) = delete;
		SrvManager& operator=(const SrvManager&) = delete;

		ID3D12Device* device_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
		uint32_t descriptorSizeSRV_ = 0;

		// 各タイプの範囲設定
		std::unordered_map<SrvHeapType, HeapRange> ranges_;
		// インデックスがどのタイプに属するか検索するためのマップ（解放時に使用）
		std::unordered_map<uint32_t, SrvHeapType> indexTypeMap_;
	};
}
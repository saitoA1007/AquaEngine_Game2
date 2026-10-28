#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <deque>
#include "Externals/DirectXTex/d3dx12.h"

namespace GameEngine {

	class RtvManager {
	public:
		RtvManager() = default;
		~RtvManager() = default;

		void Initialize(ID3D12Device* device);

		/// <summary>
		/// 外部で生成済みのリソースから DSV デスクリプタのみ作成する
		/// </summary>
		uint32_t CreateView(ID3D12Resource* resource, DXGI_FORMAT format);

		/// <summary>
		/// インデックスを削除
		/// </summary>
		/// <param name="index"></param>
		void ReleaseIndex(uint32_t index);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;

		uint32_t GetDescriptorSize() const { return descriptorSizeRTV_; }
			
	private:
		RtvManager(const RtvManager&) = delete;
		RtvManager& operator=(const RtvManager&) = delete;

		ID3D12Device* device_ = nullptr;

		// rtvヒープ
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;

		// 最大RTV数
		uint32_t maxRtvCount_ = 128;
		uint32_t descriptorSizeRTV_ = 0;
		uint32_t nextRtvIndex_ = 0;

		// 解放されたインデックスのリスト
		std::deque<uint32_t> freeIndices_;

	private:

		/// <summary>
		/// 空きインデックスを確保して返す
		/// </summary>
		/// <returns></returns>
		uint32_t AllocateIndex();

		/// <summary>
		/// インデックスに対応するCPUハンドルにRTVを作成する
		/// </summary>
		/// <param name="index"></param>
		/// <param name="resource"></param>
		/// <param name="format"></param>
		void CreateRtvDescriptor(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT format);
	};
}
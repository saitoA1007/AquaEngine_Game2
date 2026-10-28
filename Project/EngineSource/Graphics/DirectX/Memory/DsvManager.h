#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <deque>
#include "Externals/DirectXTex/d3dx12.h"

namespace GameEngine {

    class DsvManager {
    public:
        DsvManager() = default;
        ~DsvManager() = default;

        void Initialize(ID3D12Device* device);

        /// <summary>
        /// 外部で生成済みのリソースから DSV デスクリプタのみ作成する
        /// </summary>
        uint32_t CreateView(ID3D12Resource* resource, DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);

        /// <summary>
        /// リソースの解放とインデックスの回収
        /// </summary>
        void ReleaseIndex(uint32_t index);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;

        /// <summary>
        /// DsvContextのフォーマットに対応したTypelessフォーマットを返す
        /// </summary>
        /// <param name="dsvFormat"></param>
        /// <returns></returns>
        static DXGI_FORMAT ToTypelessFormat(DXGI_FORMAT dsvFormat);

        /// <summary>
        /// フォーマットに対応したSRVフォーマットを返す
        /// </summary>
        /// <param name="dsvFormat"></param>
        /// <returns></returns>
        static DXGI_FORMAT ToSrvFormat(DXGI_FORMAT dsvFormat);

    private:
        DsvManager(const DsvManager&) = delete;
        DsvManager& operator=(const DsvManager&) = delete;

        ID3D12Device* device_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;

        uint32_t maxDsvCount_ = 15;
        uint32_t descriptorSizeDSV_ = 0;
        uint32_t nextDsvIndex_ = 0;

        std::deque<uint32_t> freeIndices_;
    private:
        /// <summary>
        /// 空きインデックスを確保して返す
        /// </summary>
        /// <returns></returns>
        uint32_t AllocateIndex();

        /// <summary>
        /// インデックスに対応するCPUハンドルにDSVを作成する
        /// </summary>
        /// <param name="index"></param>
        /// <param name="resource"></param>
        /// <param name="dsvFormat"></param>
        void CreateDsvDescriptor(uint32_t index, ID3D12Resource* resource, DXGI_FORMAT dsvFormat);
    };
}

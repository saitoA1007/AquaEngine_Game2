#pragma once
#include "GpuResource.h"
#include "SrvManager.h"
#include "Externals/DirectXTex/d3dx12.h"

namespace GameEngine {

    /// <summary>
    /// GpuResourceクラスにsrvの作成を拡張した汎用クラス
    /// </summary>
    class SrvResource : public GpuResource {
    public:
        virtual ~SrvResource() = default;

        /// <summary>
        /// 静的初期化
        /// </summary>
        static void StaticInitialize(SrvManager* srvManager) {
            srvManager_ = srvManager;
        }

        uint32_t GetSrvIndex() const { return srvIndex_; }

        CD3DX12_CPU_DESCRIPTOR_HANDLE GetSrvCpuHandle() const { return srvCpuHandle_; }
        CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return srvGpuHandle_; }

    protected:
        // srvManager
        static SrvManager* srvManager_;

        uint32_t srvIndex_ = 0;

        // ハンドル
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle_{};
        CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_{};
    };
}
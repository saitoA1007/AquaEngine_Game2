#pragma once
#include "SrvResource.h"
#include <string>
#include "Externals/DirectXTex/DirectXTex.h"
#include "Externals/DirectXTex/d3dx12.h"

namespace GameEngine {

    /// <summary>
    /// テクスチャのデータ保持
    /// </summary>
    class Texture : public SrvResource {
    public:
        Texture() = default;
        ~Texture();

        // テクスチャの作成
        void Create(const std::string& filePath, ID3D12GraphicsCommandList* cmdList, bool isSRGB = true, bool useCache = true);
       
        // ゲッター
        uint32_t GetSrvIndex() const { return srvIndex_; }
        const std::string& GetFileName() const { return fileName_; }
        const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetSrvHandleGPU()  const { return srvHandleGPU_; }
        const CD3DX12_CPU_DESCRIPTOR_HANDLE& GetSrvHandleCPU()  const { return srvHandleCPU_; }

    private:
        // コピー禁止
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        std::string fileName_;
        DirectX::ScratchImage mipImage_;

        uint32_t srvIndex_ = 0;
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_;
        CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_;
    };
}
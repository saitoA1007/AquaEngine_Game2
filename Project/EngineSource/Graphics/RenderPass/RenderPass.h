#pragma once
#include <d3d12.h>
#include <string>
#include "Externals/DirectXTex/d3dx12.h"
#include "RenderTexture.h"
#include "Vector4.h"

namespace GameEngine {

    class RenderPass {
    public:

        RenderPass(const std::string& name, ID3D12GraphicsCommandList* commandList, RenderTexture* renderTexture, Vector4 clearColor);

        // 描画前処理
        void PrePass();

        // ターゲットを設定する
        void SetRenderTarget();

        // 描画後処理
        void PostPass();

        // RTV描画後にUAVとして使用するためUAV状態へ遷移する
        void SwitchToUnorderedAccess();

        // UAV書き込み後にGPU側の書き込みを完了させるバリアを挿入する
        void InsertUavBarrier();

        // 深度をコピーするため
        void SetOnlyDsvRenderTarget();

        // レンダーパスのクリアをおこなう
        void ClearRenderPass();

        // srvIndexを取得
        uint32_t GetSrvIndex() const { return renderTexture_->GetSrvIndex(); }
        uint32_t GetDepthSrvIndex() const { return renderTexture_->GetDepthSrvIndex(); }
        uint32_t GetUavIndex() const { return renderTexture_->GetUavIndex(); }

        CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvHandle();

        const D3D12_CPU_DESCRIPTOR_HANDLE& GetRtvHandle() const { return renderTexture_->GetRtvHandle(); }
        const D3D12_CPU_DESCRIPTOR_HANDLE& GetDsvHandle() const { return renderTexture_->GetDsvHandle(); }

        // 名前を取得
        const std::string GetName() const { return name_; }

        /// <summary>
        /// 描画範囲の設定
        /// </summary>
        /// <param name="width"></param>
        /// <param name="height"></param>
        void SetDrawRange(const uint32_t& width, const uint32_t& height, const uint32_t& left = 0, const uint32_t& top = 0);

    private:

        RenderTexture* renderTexture_ = nullptr;

        ID3D12GraphicsCommandList* commandList_ = nullptr;
        D3D12_VIEWPORT viewport_{};
        D3D12_RECT scissorRect_{};

        // クリア色
        float clearColor_[4];

        // パスの名前
        std::string name_;

        // レンダーモード
        RenderTextureMode mode_;
    };
}
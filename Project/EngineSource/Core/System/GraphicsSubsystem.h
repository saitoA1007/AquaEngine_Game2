#pragma once
#include "IEngineSubsystem.h"

#include "ImGuiManager.h"

#include "GraphicsDevice.h"
#include "DXC.h"
#include "RenderPipeline.h"
#include "RenderQueue.h"
#include "DebugRenderer.h"
#include "RenderTextureManager.h"
#include "BufferRefManager.h"
#include "SceneRenderManager.h"

// あとで削除するべき処理
#include "PostProcess/CopyPSO.h"
#include "RaytracingPipeline.h"

namespace GameEngine {

    /// <summary>
    /// 描画機能
    /// </summary>
    class GraphicsSubsystem : public IEngineSubsystem {
    public:
        GraphicsSubsystem();
        ~GraphicsSubsystem();

        void Initialize() override;
        void Finalize()   override;

        // コンテキストを設定（Initialize 前に呼ぶ）
        void SetContext(const EngineContext& ctx) { context_ = ctx; }

        /// フレーム制御
        void BeginFrame();
        void EndFrame();

    public:

        /// アクセサ
        GraphicsDevice* GetGraphicsDevice() const { return graphicsDevice_.get(); }
        DXC* GetDXC() const { return dxc_.get(); }
        PSOManager* GetPSOManager() const { return psoManager_.get(); }
        RenderPipeline* GetRenderPipeline() const { return renderPipeline_.get(); }
        RenderPassController* GetRenderPassCtrl() const { return renderPassController_.get(); }
        RenderQueue* GetRenderQueue() const { return renderQueue_.get(); }
        DebugRenderer* GetDebugRenderer() const { return debugRenderer_.get(); }
        ImGuiManager* GetImGuiManager() const { return imGuiManager_.get(); }
        PostEffectManager* GetPostEffectManager() const { return postEffectManager_.get(); }
        RaytracingPipeline* GetRaytracingPipeline() const { return raytracingPipeline_.get(); }

    private:
        // imGui機能
        std::unique_ptr<ImGuiManager> imGuiManager_;

        // DirectXのコア機能
        std::unique_ptr<GraphicsDevice> graphicsDevice_;
        std::unique_ptr<DXC> dxc_;
        // 描画の流れを管理
        std::unique_ptr<RenderPipeline> renderPipeline_;
        // 描画処理
        std::unique_ptr<RenderQueue> renderQueue_;
        // シーンの描画管理機能
        std::unique_ptr<SceneRenderManager> sceneRenderManager_;
        // ポストエフェクト
        std::unique_ptr<PostEffectManager> postEffectManager_;
        // デバック描画機能
        std::unique_ptr<DebugRenderer> debugRenderer_;
        // レンダーテクスチャの管理機能
        std::unique_ptr<RenderTextureManager> renderTextureManager_;
        // レンダーパスの管理機能
        std::unique_ptr<RenderPassController> renderPassController_;
        // pso管理機能
        std::unique_ptr<PSOManager> psoManager_;
        // レイトレーシングの描画環境構築機能
        std::unique_ptr<RaytracingPipeline> raytracingPipeline_;

        // バッファのアクセスデータ管理機能
        std::unique_ptr<BufferRefManager> bufferRefManager_;

        std::unique_ptr<CopyPSO> copyPSO_;

    private:
        void InitializePSO();
    };
}
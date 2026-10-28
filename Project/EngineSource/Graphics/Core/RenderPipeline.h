#pragma once
#include <Windows.h>
#include "GraphicsDevice.h"
#include "FrameRateController.h"
#include "SrvManager.h"
#include "RenderPass/RenderPassController.h"
#include "PostProcess/CopyPSO.h"
#include "PostProcess/PostEffectManager.h"
#include "SceneRenderManager.h"

namespace GameEngine {

    // 前方宣言
    class ImGuiManager;

    /// <summary>
    /// レンダリングパイプライン全体を総合管理するクラス
    /// </summary>
    class RenderPipeline final {
    public:
        RenderPipeline() = default;
        ~RenderPipeline() = default;

        /// <summary>
        /// レンダリングパイプラインの初期化
        /// </summary>
        /// <param name="hwnd">ウィンドウハンドル</param>
        /// <param name="width">画面幅</param>
        /// <param name="height">画面高さ</param>
        /// <param name="srvManager">SRVマネージャー</param>
        void Initialize(GraphicsDevice* graphicsDevice, SceneRenderManager* sceneRenderManager, PostEffectManager* postEffectManager, RenderPassController* renderPassController);

        /// <summary>
        /// 描画開始処理
        /// </summary>
        void BeginFrame();

        /// <summary>
        /// 描画終了処理
        /// </summary>
        /// <param name="imGuiManager"></param>
        void EndFrame(ImGuiManager* imGuiManager);

    public:

        /// <summary>
        /// PostEffect用のPSOを設定
        /// </summary>
        /// <param name="copyPSO"></param>
        void SetCopyPSO(CopyPSO* copyPSO) { copyPSO_ = copyPSO; }

    private:
        RenderPipeline(const RenderPipeline&) = delete;
        RenderPipeline& operator=(const RenderPipeline&) = delete;

        // DirectXのコア機能
        GraphicsDevice* graphicsDevice_ = nullptr;
        // Fps管理機能
        std::unique_ptr<FrameRateController> frameRateController_;

        // シーン描画管理
        SceneRenderManager* sceneRenderManager_ = nullptr;
        // ポストエフェクト描画管理
        PostEffectManager* postEffectManager_ = nullptr;

        RenderPassController* renderPassController_ = nullptr;
        CopyPSO* copyPSO_ = nullptr;

    private:

        void TransitionBackBuffer(D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
    };
}
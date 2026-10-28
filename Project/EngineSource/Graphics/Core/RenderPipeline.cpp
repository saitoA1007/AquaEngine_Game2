#include "pch.h"
#include "RenderPipeline.h"
#include "ImGuiManager.h"
#include "LogManager.h"
#include "ResourceGarbageCollector.h"
#include "Debug/PixMarker.h"
#include "Debug/PixCapture.h"
using namespace GameEngine;

void RenderPipeline::Initialize(GraphicsDevice* graphicsDevice, SceneRenderManager* sceneRenderManager, PostEffectManager* postEffectManager, RenderPassController* renderPassController) {
    LogManager::GetInstance().Log("RenderPipeline start Initialize");

    // シーン描画管理
    sceneRenderManager_ = sceneRenderManager;

    // ポストエフェクトの管理
    postEffectManager_ = postEffectManager;

    renderPassController_ = renderPassController;

    // DirectXのコア機能を取得
    graphicsDevice_ = graphicsDevice;

    // FPS固定初期化
    frameRateController_ = std::make_unique<FrameRateController>();
    frameRateController_->InitializeFixFPS();

    LogManager::GetInstance().Log("RenderPipeline end Initialize\n");
}

void RenderPipeline::BeginFrame() {
    // PIXのプログラマブルキャプチャが予約されていればここで開始する
    PixCapture::GetInstance().BeginFrame();

    // PIX上でこのフレーム全体を括るイベントを開始
    PixBeginEvent(graphicsDevice_->GetCommandList(), PixColor::Frame, "Frame");

      // ヒープを設定する
    ID3D12DescriptorHeap* descriptorHeaps[] = { graphicsDevice_->GetSrvManager()->GetSRVHeap() };
    graphicsDevice_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);

    {
        PIX_SCOPED_EVENT(graphicsDevice_->GetCommandList(), PixColor::Scene, "SceneBegin(Clear)");
        // クリア
        sceneRenderManager_->Begin();
    }
}

void RenderPipeline::EndFrame(ImGuiManager* imGuiManager) {

    {
        PIX_SCOPED_EVENT(graphicsDevice_->GetCommandList(), PixColor::Scene, "SceneRender");
        // シーン描画を実行
        sceneRenderManager_->Execute();
    }

    {
        PIX_SCOPED_EVENT(graphicsDevice_->GetCommandList(), PixColor::PostEffect, "PostEffect");
        // ポストエフェクトを実行
        postEffectManager_->Execute();
    }

    // レンダーパス側で閉じ忘れたイベントがあればここで全て閉じる
    renderPassController_->CloseAllPixEvents();

    {
        PIX_SCOPED_EVENT(graphicsDevice_->GetCommandList(), PixColor::Present, "PresentPass");

        /// 最終結果を描画する
        // バックバッファをレンダーターゲットに遷移
        TransitionBackBuffer(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        // バックバッファを描画先に設定
        uint32_t backBufferIndex = graphicsDevice_->GetBackBufferIndex();
        auto rtvHandle = graphicsDevice_->GetSwapChainRTVHandle(backBufferIndex);
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = graphicsDevice_->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();
        graphicsDevice_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

        // ビューポート、シザーを設定
        graphicsDevice_->GetCommandList()->RSSetViewports(1, &graphicsDevice_->GetViewport());
        graphicsDevice_->GetCommandList()->RSSetScissorRects(1, &graphicsDevice_->GetScissorRect());

#ifdef USE_IMGUI
        {
            PIX_SCOPED_EVENT(graphicsDevice_->GetCommandList(), PixColor::UI, "ImGui");
            // ImGuiを描画
            imGuiManager->Draw();
        }
#else
        // ポストプロセス結果を描画
        copyPSO_->Draw(graphicsDevice_->GetCommandList(), renderPassController_->GetSrvHandle(renderPassController_->GetPresentPass()));
#endif

        // バックバッファをPresentに遷移
        TransitionBackBuffer(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    }

    // フレーム全体のPIXイベントを閉じる
    PixEndEvent(graphicsDevice_->GetCommandList());

    // コマンドリストの内容を確定させる。すべてのコマンドを積んでからcloseにすること
    graphicsDevice_->CloseCommandList();
    // GPUにコマンドリストの実行を行わせる
    graphicsDevice_->ExecuteCommand();

    // GPUとOSに画面の交換を行うように通知する
    graphicsDevice_->Present();

    // TDRはこのタイミングで表面化するため、GPU待ちに入る前に状態を確認してDREDのログを残す
    graphicsDevice_->CheckDeviceStatus();

    // Presentが終わったフレームを1枚としてカウントし、必要ならキャプチャを確定させる
    PixCapture::GetInstance().EndFrame();

    // GPUを待つ
    graphicsDevice_->WaitForGPU();

    // 次のフレーム用にコマンドリストを準備
    graphicsDevice_->ResetCommandList();

    // FPS固定
    frameRateController_->UpdateFixFPS();

    // 使用していないリソースを削除する
    ResourceGarbageCollector::GetInstance().ProcessCompletedResources();
}

void RenderPipeline::TransitionBackBuffer(D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
    uint32_t backBufferIndex = graphicsDevice_->GetBackBufferIndex();
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = graphicsDevice_->GetSwapChainResource(backBufferIndex);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;
    graphicsDevice_->GetCommandList()->ResourceBarrier(1, &barrier);
}
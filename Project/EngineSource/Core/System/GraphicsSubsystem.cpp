#include "pch.h"
#include "GraphicsSubsystem.h"
#include "GpuResource.h"
#include "SrvResource.h"
#include "BufferRefResource.h"
#include "Sprite.h"
#include "SpriteRenderer.h"
#include "ModelRenderer.h"
#include "CoreSubsystem.h"
#include "Animator.h"
#include "ParticleBehaviorGPU.h"
#include "FractureStaticMesh.h"
#include "Debug/PixCapture.h"
#include "ScopedTimer.h"
using namespace GameEngine;

GraphicsSubsystem::GraphicsSubsystem() = default;
GraphicsSubsystem::~GraphicsSubsystem() = default;

void GraphicsSubsystem::Initialize() {
    ScopedTimer total("LoadAllGraphics (Total)");

    auto* windowsApp = context_.core->GetWindowsApp();

    // PIXのGPUキャプチャ機能を準備する。
    PixCapture::GetInstance().Initialize();

    // DirectXの機能を生成
    graphicsDevice_ = std::make_unique<GraphicsDevice>();
    graphicsDevice_->Initialize(
        windowsApp->GetHwnd(),
        windowsApp->GetClientWidth(),
        windowsApp->GetClientHeight());

    // HUD表示の設定は、スワップチェーン生成後に行う。
    PixCapture::GetInstance().SetHudVisible(false);

    // dxcCompilerの初期化
    dxc_ = std::make_unique<DXC>();
    dxc_->Initialize();

    // GPUリソースの静的初期化
    GpuResource::StaticInitialize(graphicsDevice_->GetDevice());
    SrvResource::StaticInitialize(graphicsDevice_->GetSrvManager());

    // バッファのアクセスデータ管理機能の初期化
    bufferRefManager_ = std::make_unique<BufferRefManager>();
    bufferRefManager_->Initialize();

    // アクセスデータ管理機能を登録する
    BufferRefResource::StaticInitialize(bufferRefManager_.get(), graphicsDevice_->GetSrvManager()->GetStartSrvIndex(SrvHeapType::Buffer));

    {
        ScopedTimer t("PSO");

        // PSO作成
        //InitializePSO();

        { ScopedTimer t1("CopyPSO"); InitializePSO(); }

        // PSO管理機能の作成
        psoManager_ = std::make_unique<PSOManager>();
        psoManager_->Initialize(graphicsDevice_->GetDevice(), dxc_.get());
        { ScopedTimer t2("DefaultLoadPSO"); psoManager_->DefaultLoadPSO(); }
        { ScopedTimer t3("PostEffectPSO");  psoManager_->DefaultLoadPostEffectPSO(); }
        //psoManager_->DefaultLoadPSO();
        //psoManager_->DefaultLoadPostEffectPSO();
        
        ScopedTimer t4("Raytracing");
        // レイトレーシング用のパイプライン
        raytracingPipeline_ = std::make_unique<RaytracingPipeline>();
        raytracingPipeline_->Initialize(graphicsDevice_->GetDevice(), graphicsDevice_->GetSrvManager(), dxc_.get());
    }

    // レンダーテクスチャ機能を生成
    renderTextureManager_ = std::make_unique<RenderTextureManager>();
    renderTextureManager_->Initialize(graphicsDevice_->GetRtvManager(), graphicsDevice_->GetDsvManager(), graphicsDevice_->GetDevice());

    // レンダーパスの管理機能
    renderPassController_ = std::make_unique<RenderPassController>();
    renderPassController_->Initialize(renderTextureManager_.get(), graphicsDevice_->GetCommandList());

    // 描画コマンド発行機能
    renderQueue_ = std::make_unique<RenderQueue>();
    renderQueue_->Initialize();

    // シーン描画の管理
    sceneRenderManager_ = std::make_unique<SceneRenderManager>();
    sceneRenderManager_->Initialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager(), psoManager_.get(),
        renderPassController_.get(), raytracingPipeline_.get(), bufferRefManager_.get(),renderQueue_.get());

    // ポストエフェクトマネージャーの初期化
    postEffectManager_ = std::make_unique<PostEffectManager>();
    postEffectManager_->Initialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager(), psoManager_.get(), renderPassController_.get());

    // 描画の流れを管理するクラスを初期化
    renderPipeline_ = std::make_unique<RenderPipeline>();
    renderPipeline_->Initialize(graphicsDevice_.get(), sceneRenderManager_.get(), postEffectManager_.get(), renderPassController_.get());
    renderPipeline_->SetCopyPSO(copyPSO_.get());

    // ImGuiの初期化
    imGuiManager_ = std::make_unique<ImGuiManager>();
    imGuiManager_->Initialize(graphicsDevice_->GetDevice(), graphicsDevice_->GetCommandList(), graphicsDevice_->GetSwapChainDesc(),
        windowsApp, graphicsDevice_->GetSrvManager());

    // デバックレンダラー
    debugRenderer_ = std::make_unique<DebugRenderer>();

    // Rendererの静的初期化
    Sprite::StaticInitialize(windowsApp->kWindowWidth, windowsApp->kWindowHeight);
    SpriteRenderer::StaticInitialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager());
    ModelRenderer::StaticInitialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager());
    // アニメーション用の静的初期化
    Animator::StaticInitialize(graphicsDevice_->GetCommandList(), psoManager_.get());
    // Csパーティクル用
    ParticleBehaviorGPU::StaticInitialize(graphicsDevice_->GetCommandList(), psoManager_.get());
    // 破壊オブジェクトの静的なメッシュのBLAS構築用
    FractureStaticMesh::StaticInitialize(graphicsDevice_->GetCommandList());
}

void GraphicsSubsystem::InitializePSO() {
    // CopyPSOの初期化
    copyPSO_ = std::make_unique<CopyPSO>();
    copyPSO_->Initialize(graphicsDevice_->GetDevice(), L"Resources/Shaders/PostEffect/FullScreen.VS.hlsl", L"Resources/Shaders/PostEffect/Copy.PS.hlsl", dxc_.get());
}

void GraphicsSubsystem::Finalize() {
    imGuiManager_->Finalize();
}

void GraphicsSubsystem::BeginFrame() {
    renderPipeline_->BeginFrame();
}

void GraphicsSubsystem::EndFrame() {
    renderPipeline_->EndFrame(imGuiManager_.get());
}
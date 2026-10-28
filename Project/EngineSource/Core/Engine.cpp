#include "pch.h"
#include "Engine.h"
#include "CrashHandle.h"
#include "LogManager.h"
#include "ScopedTimer.h"
#include "Debug/PixCapture.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")

using namespace GameEngine;

void Engine::RunEngine(HINSTANCE& hInstance, const std::function<void(SceneRegistry&)>& setupScenes) {
    // 初期化
    Initialize(hInstance, setupScenes);
    // 非表示のままFPSが安定するまでフレームを回す
    const bool isReady = WarmUp();
    // ゲームウィンドウを表示してからロードウィンドウを閉じる
    if (isReady) {
        core_->GetWindowsApp()->ShowGameWindow();
    }
    loadingWindow_.Close();
    // 更新処理
    if (isReady) {
        MainLoop();
    }
    // 終了処理
    Finalize();
}

void Engine::Initialize(HINSTANCE hInstance, const std::function<void(SceneRegistry&)>& setupScenes) {
    SetUnhandledExceptionFilter(ExportDump);
    LogManager::GetInstance().Create();
    ScopedTimer total("AquaEngine Load Initialize (Total)");

    // 重い初期化の前にロードウィンドウを表示する
    loadingWindow_.Show(L"AquaEngine", L"Now_Loading");

    // サブシステムを生成
    core_ = std::make_unique<CoreSubsystem>(CoreSubsystemDesc{ L"AquaEngine", 1280, 720, hInstance });
    graphics_ = std::make_unique<GraphicsSubsystem>();
    resource_ = std::make_unique<ResourceSubsystem>();
    input_ = std::make_unique<InputSubsystem>();
    scene_ = std::make_unique<SceneSubsystem>();
#ifdef USE_IMGUI
    editor_ = std::make_unique<EditorSubsystem>();
#endif

    // 初期化順を登録。登録順に実行される
    subsystemRegistry_.Register("Core", core_.get());
    subsystemRegistry_.Register("Graphics", graphics_.get());
    subsystemRegistry_.Register("Resource", resource_.get());
    subsystemRegistry_.Register("Input", input_.get());
    subsystemRegistry_.Register("Scene", scene_.get());
#ifdef USE_IMGUI
    subsystemRegistry_.Register("Editor", editor_.get());
#endif

    // エンジン機能のcontexを作成
    BuildEngineContext();
    graphics_->SetContext(context_);
    resource_->SetContext(context_);
    input_->SetContext(context_);
    scene_->SetContext(context_);
#ifdef USE_IMGUI
    editor_->SetContext(context_);
#endif

    // 初期化をおこなう
    subsystemRegistry_.InitializeAll();

    // ゲーム側のシーンを登録する
    setupScenes(*scene_->GetSceneRegistry());
    // シーンに各機能を設定
    BuildSceneServices();
    // シーンの初期化
    scene_->SceneInitialize();

    // エディタへのシーン情報設定
#ifdef USE_IMGUI
    auto* request = scene_->GetSceneChangeRequest();
    request->SetCurrentSceneName(scene_->GetCurrentSceneName());
    request->SetSceneNames(scene_->GetSceneRegistry()->GetSceneNames());
#endif
}

void Engine::BuildEngineContext() {
    context_.core = core_.get();
    context_.graphics = graphics_.get();
    context_.resource = resource_.get();
    context_.input = input_.get();
    context_.scene = scene_.get();
}

void Engine::BuildSceneServices() {
    // シーンに入力機能を設定
    IScene::SetInput(input_->GetInput(), input_->GetInputCommand());
    // シーンにリソース管理機能を設定
    IScene::SetResourceManager(resource_->GetTextureManager(), resource_->GetModelManager(), resource_->GetAnimationManager(), 
        scene_->GetGameObjectManager(), resource_->GetEffectsManager(), resource_->GetFontManager());
    // シーンに描画機能を設定
    IScene::SetRender(graphics_->GetRenderPassCtrl(), graphics_->GetRenderQueue(),graphics_->GetPostEffectManager());
    // デバック描画機能を設定
    IScene::SetDebug(graphics_->GetDebugRenderer());
}

void Engine::MainLoop() {
    while (!core_->IsWindowCloseRequested()) {
        RunFrame();
    }
}

void Engine::RunFrame() {
    PreUpdate();

    if (isActiveUpdate_ && !isPause_) {
        scene_->UpdateGameplay();
    } else {
        scene_->UpdateDebug();
    }

    scene_->GetCollisionManager()->DebugDraw(graphics_->GetDebugRenderer());

    PostUpdate();

    PreDraw();
    scene_->Draw();
#ifdef USE_IMGUI
    graphics_->GetRenderQueue()->SubmitDebugLine(graphics_->GetDebugRenderer());
#endif
    PostDraw();
}

void Engine::PreUpdate() {
    // デバイスチェック
    graphics_->GetGraphicsDevice()->CheckDeviceStatus();
    core_->Update(); // FPS
    input_->Update(); // 入力処理

    // F11でPIXのGPUキャプチャを取得する
    if (input_->GetInput()->TriggerKey(DIK_F11)) {
        PixCapture::GetInstance().RequestCapture(1);
    }

    graphics_->GetImGuiManager()->BeginFrame();

    // ヒープを設定する
    graphics_->BeginFrame();
#ifdef USE_IMGUI
    // デバック描画をリセット
    graphics_->GetDebugRenderer()->Clear();
    // エディターの更新処理
    editor_->Update();

    isActiveUpdate_ = editor_->IsActiveUpdate();
    isPause_ = editor_->IsPause();

    // 更新停止中にリセットが必要なら実行
    if (isActiveUpdate_) {
        isReset_ = false;
    } else if (!isReset_) {
        scene_->ResetCurrentScene();
        isReset_ = true;
    }

    // シーン切り替えリクエストを処理
    auto* request = scene_->GetSceneChangeRequest();
    if (request->HasChangeRequest()) {
        scene_->GetCollisionManager()->ClearList();
        scene_->ChangeScene(request->GetRequestScene());
        editor_->SceneReset();
        request->ClearChangeRequest();
        request->SetCurrentSceneName(scene_->GetCurrentSceneName());
    }

    scene_->GetSceneManager()->DebugUpdate();
    scene_->GetGameObjectManager()->DebugUpdateAll();
#endif
}

void Engine::PostUpdate() {
    graphics_->GetImGuiManager()->EndFrame();
}

void Engine::PreDraw() {
    //graphics_->BeginFrame();
}

void Engine::PostDraw() {
    graphics_->GetDebugRenderer()->Update();
    graphics_->EndFrame();
}

void Engine::Finalize() {
    subsystemRegistry_.FinalizeAll();
}

bool Engine::WarmUp() {
    loadingWindow_.SetMessage(L"Preparing");

    using Clock = std::chrono::steady_clock;
    const Clock::time_point startTime = Clock::now();
    Clock::time_point prevTime = startTime;
    int frameCount = 0;
    int stableFrameCount = 0;

    while (true) {
        if (core_->IsWindowCloseRequested()) {
            return false;
        }

        RunFrame();

        // 1フレームにかかった時間を計測
        const Clock::time_point now = Clock::now();
        const float frameTime = std::chrono::duration<float>(now - prevTime).count();
        prevTime = now;
        frameCount++;

        if (frameCount > kWarmUpIgnoreFrames) {
            // 規定時間内に収まったフレームが連続したら安定とみなす
            stableFrameCount = (frameTime <= kStableFrameTime) ? stableFrameCount + 1 : 0;
            if (stableFrameCount >= kRequiredStableFrames) {
                break;
            }
        }

        // 安定しない環境でも一定時間で表示する
        if (now - startTime >= kWarmUpTimeout) {
            LogManager::GetInstance().Log("WarmUp frame " + std::to_string(frameCount) + " : " + std::to_string(frameTime * 1000.0f) + " ms");
            break;
        }
    }
    return true;
}

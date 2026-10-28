#pragma once
#include "SubsystemRegistry.h"
#include "EngineContext.h"
#include "LoadingWindow.h"

#include "CoreSubsystem.h"
#include "GraphicsSubsystem.h"
#include "ResourceSubsystem.h"
#include "InputSubsystem.h"
#include "SceneSubsystem.h"
#ifdef USE_IMGUI
#include "EditorSubsystem.h"
#endif
#include <memory>
#include <chrono>

namespace GameEngine {

    /// <summary>
    /// エンジン
    /// </summary>
    class Engine final {
    public:
        Engine() = default;
        ~Engine() = default;
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void RunEngine(HINSTANCE& hInstance, const std::function<void(SceneRegistry&)>& setupScenes);

    private:
        std::unique_ptr<CoreSubsystem> core_;
        std::unique_ptr<GraphicsSubsystem> graphics_;
        std::unique_ptr<ResourceSubsystem> resource_;
        std::unique_ptr<InputSubsystem> input_;
        std::unique_ptr<SceneSubsystem> scene_;
#ifdef USE_IMGUI
        std::unique_ptr<EditorSubsystem> editor_;
#endif
        EngineContext context_;

        // システムの管理
        SubsystemRegistry subsystemRegistry_;

        // ロード中に表示するウィンドウ
        LoadingWindow loadingWindow_;

        // シーンの更新状態を管理
        bool isActiveUpdate_ = true;
        bool isPause_ = false;
        bool isReset_ = true;

        // 計測から除外する最初のフレーム数
        const int kWarmUpIgnoreFrames = 10;
        // 安定とみなすのに必要な連続フレーム数
        const int kRequiredStableFrames = 30;
        // 安定とみなす1フレームの最大時間
        const float kStableFrameTime = 1.0f / 55.0f;
        // 安定しなくても表示するまでの最大待ち時間
        const std::chrono::seconds kWarmUpTimeout{ 1 };

    private:
        void Initialize(HINSTANCE hInstance, const std::function<void(SceneRegistry&)>& setupScenes);
        void Finalize();
        void MainLoop();
        void RunFrame();
        void PreUpdate();
        void PostUpdate();
        void PreDraw();
        void PostDraw();
        
        // 非表示のままフレームを回し、FPSが安定するまで待つ。終了要求があればfalse
        bool WarmUp();

        void BuildEngineContext();
        void BuildSceneServices();
    };
}
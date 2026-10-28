#pragma once
#include "IEngineSubsystem.h"
#include "WindowsApp.h"
#include "FPSCounter.h"

namespace GameEngine {

    struct CoreSubsystemDesc {
        std::wstring title = L"GameEngine";
        uint32_t     width = 1280;
        uint32_t     height = 720;
        HINSTANCE    hInstance{};
    };

    /// <summary>
    /// コアサブシステム
    /// </summary>
    class CoreSubsystem : public IEngineSubsystem {
    public:
        explicit CoreSubsystem(const CoreSubsystemDesc& desc);
        ~CoreSubsystem();

        void Initialize() override;
        void Update()     override;
        void Finalize()   override;

        // ウィンドウが閉じられたか
        bool IsWindowCloseRequested() const;

        WindowsApp* GetWindowsApp() const { return windowsApp_.get(); }
        FpsCounter* GetFpsCounter()  const { return fpsCounter_.get(); }

    private:
        CoreSubsystemDesc desc_;

        std::unique_ptr<WindowsApp> windowsApp_;
        std::unique_ptr<FpsCounter> fpsCounter_;
    };
}
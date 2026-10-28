#include "pch.h"
#include "CoreSubsystem.h"
#include "RandomGenerator.h"
using namespace GameEngine;

CoreSubsystem::CoreSubsystem(const CoreSubsystemDesc& desc)
    : desc_(desc) {
}

CoreSubsystem::~CoreSubsystem() = default;

void CoreSubsystem::Initialize() {
    // ウィンドウ生成
    windowsApp_ = std::make_unique<WindowsApp>();
    windowsApp_->CreateGameWindow(desc_.title, desc_.width, desc_.height);

    // FPS計測機能を初期化
    fpsCounter_ = std::make_unique<FpsCounter>();
    fpsCounter_->Initialize();

    // ランダム生成機能の初期化
    RandomGenerator::Initialize();
}

void CoreSubsystem::Update() {
    fpsCounter_->Update();
}

void CoreSubsystem::Finalize() {
    windowsApp_->BreakGameWindow();
}

bool CoreSubsystem::IsWindowCloseRequested() const {
    return windowsApp_->ProcessMessage();
}
#include "pch.h"
#include "InputSubsystem.h"
#include "CoreSubsystem.h"
using namespace GameEngine;

InputSubsystem::InputSubsystem() = default;
InputSubsystem::~InputSubsystem() = default;

void InputSubsystem::Initialize() {
    auto* windowsApp = context_.core->GetWindowsApp();

    // 入力処理を初期化
    input_ = std::make_unique<Input>();
    input_->Initialize(windowsApp->GetHInstance(), windowsApp->GetHwnd());
    // 入力処理のコマンドシステムを生成
    inputCommand_ = std::make_unique<InputCommand>(input_.get());
}

void InputSubsystem::Update() {
    input_->Update();
    inputCommand_->Update();
}
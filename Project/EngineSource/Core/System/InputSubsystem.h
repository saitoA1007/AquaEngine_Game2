#pragma once
#include <memory>
#include "IEngineSubsystem.h"

#include "InPut.h"
#include "InputCommand.h"

namespace GameEngine {

    /// <summary>
    /// 入力システム機能
    /// </summary>
    class InputSubsystem : public IEngineSubsystem {
    public:
        InputSubsystem();
        ~InputSubsystem();

        void Initialize() override;
        void Update()     override;

        void SetContext(const EngineContext& ctx) { context_ = ctx; }

        Input* GetInput()        const { return input_.get(); }
        InputCommand* GetInputCommand() const { return inputCommand_.get(); }

    private:
        std::unique_ptr<Input>        input_;
        std::unique_ptr<InputCommand> inputCommand_;
    };
}
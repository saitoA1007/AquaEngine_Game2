#pragma once
#include "EngineContext.h"

namespace GameEngine {

    /// <summary>
    /// エンジンサブシステムの基底クラス
    /// </summary>
    class IEngineSubsystem {
    public:
        virtual ~IEngineSubsystem() = default;

        virtual void Initialize() = 0;
        virtual void Update() {}
        virtual void Finalize() {}

    protected:
        EngineContext context_;
    };
}

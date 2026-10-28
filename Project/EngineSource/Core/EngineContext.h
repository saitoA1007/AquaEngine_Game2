#pragma once

namespace GameEngine {

    // システムの前方宣言
    class GraphicsSubsystem;
    class ResourceSubsystem;
    class InputSubsystem;
    class SceneSubsystem;
    class CoreSubsystem;

    /// <summary>
    /// サブシステム機能
    /// </summary>
    struct EngineContext 
    {
        CoreSubsystem* core = nullptr;
        GraphicsSubsystem* graphics = nullptr;
        ResourceSubsystem* resource = nullptr;
        InputSubsystem* input = nullptr;
        SceneSubsystem* scene = nullptr;
    };
}
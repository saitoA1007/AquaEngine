#pragma once

namespace GameEngine {

    class GraphicsSubsystem;
    class ResourceSubsystem;
    class InputSubsystem;
    class SceneSubsystem;
    class CoreSubsystem;

    /// <summary>
    /// サブシステム機能
    /// </summary>
    struct EngineContext {
        CoreSubsystem* core = nullptr;
        GraphicsSubsystem* graphics = nullptr;
        ResourceSubsystem* resource = nullptr;
        InputSubsystem* input = nullptr;
        SceneSubsystem* scene = nullptr;
    };
}
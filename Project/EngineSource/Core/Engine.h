#pragma once
#include "SubsystemRegistry.h"
#include "EngineContext.h"

#include "CoreSubsystem.h"
#include "GraphicsSubsystem.h"
#include "ResourceSubsystem.h"
#include "InputSubsystem.h"
#include "SceneSubsystem.h"
#ifdef USE_IMGUI
#include "EditorSubsystem.h"
#endif
#include <memory>

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

        void RunEngine(HINSTANCE& hInstance);

    private:
        std::unique_ptr<CoreSubsystem>     core_;
        std::unique_ptr<GraphicsSubsystem> graphics_;
        std::unique_ptr<ResourceSubsystem> resource_;
        std::unique_ptr<InputSubsystem>    input_;
        std::unique_ptr<SceneSubsystem>    scene_;
#ifdef USE_IMGUI
        std::unique_ptr<EditorSubsystem>   editor_;
#endif
        EngineContext context_;

        // システムの管理
        SubsystemRegistry subsystemRegistry_;

        // シーンの更新状態を管理
        bool isActiveUpdate_ = true;
        bool isPause_ = false;
        bool isReset_ = true;

    private:
        void Initialize(HINSTANCE hInstance);
        void Finalize();
        void MainLoop();
        void PreUpdate();
        void PostUpdate();
        void PreDraw();
        void PostDraw();

        void BuildEngineContext();
        void BuildSceneServices();
    };
}
#pragma once
#ifdef USE_IMGUI
#include "IEngineSubsystem.h"
#include "EngineContext.h"
#include "EditorCore.h"

namespace GameEngine {

    /// <summary>
    /// エディタシステム
    /// </summary>
    class EditorSubsystem : public IEngineSubsystem {
    public:
        void Initialize() override;
        void Update()     override;
        void Finalize()   override;

        void SetContext(const EngineContext& ctx) { context_ = ctx; }

        bool IsActiveUpdate() const;
        bool IsPause()        const;

        EditorCore* GetEditorCore() const { return editorCore_.get(); }

    private:
        EngineContext context_;

        std::unique_ptr<EditorCore> editorCore_;
    };
} 
#endif
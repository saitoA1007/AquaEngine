#pragma once
#include "IEngineSubsystem.h"
#include "EngineContext.h"

#include "TextureManager.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include "AudioManager.h"
#include "GameParamEditor.h"

namespace GameEngine {

    /// <summary>
    /// リソース管理機能
    /// </summary>
    class ResourceSubsystem : public IEngineSubsystem {
    public:
        void Initialize() override;
        void Finalize()   override;

        void SetContext(const EngineContext& ctx) { context_ = ctx; }

        /// リソースの一括ロード
        void LoadAllResources();

        TextureManager* GetTextureManager()   const { return textureManager_.get(); }
        ModelManager* GetModelManager()     const { return modelManager_.get(); }
        AnimationManager* GetAnimationManager() const { return animationManager_.get(); }
        GameParamEditor* GetGameParamEditor() const { return gameParamEditor_.get(); }
    private:
        EngineContext context_;

        // 画像データ管理
        std::unique_ptr<TextureManager> textureManager_;
        // モデルデータ管理
        std::unique_ptr<ModelManager> modelManager_;
        // アニメーションデータ管理
        std::unique_ptr<AnimationManager> animationManager_;
        // パラメータシステム
        std::unique_ptr<GameParamEditor> gameParamEditor_;
    };
}
#pragma once
#include "IEngineSubsystem.h"
#include "EngineContext.h"

#include "SceneManager.h"
#include "SceneRegistry.h"
#include "SceneChangeRequest.h"
#include "GameObjectManager.h"
#include "CollisionManager.h"

namespace GameEngine {

    /// <summary>
    /// シーンサブシステム
    /// シーン遷移・ゲームオブジェクト・当たり判定を統括する
    /// </summary>
    class SceneSubsystem : public IEngineSubsystem {
    public:
        void Initialize() override;
        void Finalize() override;

        void SetContext(const EngineContext& ctx) { context_ = ctx; }

        /// ゲームプレイ更新
        void UpdateGameplay();

        /// デバック専用更新
        void UpdateDebug();

        /// 描画コマンドを積む
        void Draw();

        /// シーンをリセット
        void ResetCurrentScene();

        /// シーンを切り替える
        void ChangeScene(const std::string& sceneName);

        // シーンを初期化
        void SceneInitialize();

        std::string GetCurrentSceneName() const;

        SceneManager* GetSceneManager() const { return sceneManager_.get(); }
        SceneRegistry* GetSceneRegistry() const { return sceneRegistry_.get(); }
        SceneChangeRequest* GetSceneChangeRequest() const { return sceneChangeRequest_.get(); }
        GameObjectManager* GetGameObjectManager() const { return gameObjectManager_.get(); }
        CollisionManager* GetCollisionManager() const { return collisionManager_.get(); }
    private:
        EngineContext context_;

        std::unique_ptr<SceneManager> sceneManager_;
        std::unique_ptr<SceneRegistry> sceneRegistry_;
        std::unique_ptr<SceneChangeRequest> sceneChangeRequest_;
        std::unique_ptr<GameObjectManager> gameObjectManager_;
        std::unique_ptr<CollisionManager> collisionManager_;
    };
}
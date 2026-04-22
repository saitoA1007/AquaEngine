#include "SceneSubsystem.h"
#include "ResourceSubsystem.h"
#include"InputSubsystem.h"
#include "Collider.h"
using namespace GameEngine;

void SceneSubsystem::Initialize() {
    // 当たり判定
    collisionManager_ = std::make_unique<CollisionManager>();
    Collider::StaticInitialize(collisionManager_.get());

    // シーン生成システム
    sceneRegistry_ = std::make_unique<SceneRegistry>();

    // ゲームオブジェクト管理
    gameObjectManager_ = std::make_unique<GameObjectManager>();

    // シーンマネージャ
    sceneManager_ = std::make_unique<SceneManager>();

    // シーン切り替えリクエスト
    sceneChangeRequest_ = std::make_unique<SceneChangeRequest>();


    testManager_ = std::make_unique<TestManager>();
    testCamera_ = std::make_unique<TestCamera>(context_.input->GetInput());
}

void SceneSubsystem::SceneInitialize() {
    sceneManager_->Initialize(sceneRegistry_.get(), context_.resource->GetGameParamEditor(), gameObjectManager_.get());
}

void SceneSubsystem::UpdateGameplay() {
    gameObjectManager_->UpdateAll();
    sceneManager_->Update();
    collisionManager_->CheckAllCollisions();
}

void SceneSubsystem::UpdateDebug() {
    sceneManager_->DebugSceneUpdate();
}

void SceneSubsystem::Draw() {
    sceneManager_->Draw();
    gameObjectManager_->DrawAll();
}

void SceneSubsystem::ResetCurrentScene() {
    // シーンの初期化
    sceneManager_->ResetCurrentScene();
    // ゲームオブジェクトを初期化
    gameObjectManager_->InitializeAll();
}

void SceneSubsystem::ChangeScene(const std::string& sceneName) {
    sceneManager_->ChangeScene(sceneName);
}

std::string SceneSubsystem::GetCurrentSceneName() const {
    return sceneManager_->GetCurrentSceneName();
}

void SceneSubsystem::Finalize() {
 
}
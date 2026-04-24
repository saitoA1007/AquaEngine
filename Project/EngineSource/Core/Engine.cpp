#include "Engine.h"
#include "Application/Scene/Register/SetUpScenes.h"
#include "ResourceLeakChecker.h"
#include "CrashHandle.h"
#include "LogManager.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")

using namespace GameEngine;

void Engine::RunEngine(HINSTANCE& hInstance) {
    // 初期化
    Initialize(hInstance);
    // 更新処理
    MainLoop();
    // 終了処理
    Finalize();
}

void Engine::Initialize(HINSTANCE hInstance) {
    SetUnhandledExceptionFilter(ExportDump);
    LogManager::GetInstance().Create();

    // サブシステムを生成
    core_ = std::make_unique<CoreSubsystem>(CoreSubsystemDesc{ L"AquaEngine", 1280, 720, hInstance });
    graphics_ = std::make_unique<GraphicsSubsystem>();
    resource_ = std::make_unique<ResourceSubsystem>();
    input_ = std::make_unique<InputSubsystem>();
    scene_ = std::make_unique<SceneSubsystem>();
#ifdef USE_IMGUI
    editor_ = std::make_unique<EditorSubsystem>();
#endif

    // 初期化順を登録。登録順に実行される
    subsystemRegistry_.Register("Core", core_.get());
    subsystemRegistry_.Register("Graphics", graphics_.get());
    subsystemRegistry_.Register("Resource", resource_.get());
    subsystemRegistry_.Register("Input", input_.get());
    subsystemRegistry_.Register("Scene", scene_.get());
#ifdef USE_IMGUI
    subsystemRegistry_.Register("Editor", editor_.get());
#endif

    // エンジン機能のcontexを作成
    BuildEngineContext();
    graphics_->SetContext(context_);
    resource_->SetContext(context_);
    input_->SetContext(context_);
    scene_->SetContext(context_);
#ifdef USE_IMGUI
    editor_->SetContext(context_);
#endif

    // 初期化をおこなう
    subsystemRegistry_.InitializeAll();

    // シーンを登録する
    SetupScenes(*scene_->GetSceneRegistry());
    // シーンに各機能を設定
    BuildSceneServices();
    // シーンの初期化
    scene_->SceneInitialize();

    // エディタへのシーン情報設定
#ifdef USE_IMGUI
    auto* request = scene_->GetSceneChangeRequest();
    request->SetCurrentSceneName(scene_->GetCurrentSceneName());
    request->SetSceneNames(scene_->GetSceneRegistry()->GetSceneNames());
#endif
}

void Engine::BuildEngineContext() {
    context_.core = core_.get();
    context_.graphics = graphics_.get();
    context_.resource = resource_.get();
    context_.input = input_.get();
    context_.scene = scene_.get();
}

void Engine::BuildSceneServices() {
    // シーンに入力機能を設定
    IScene::SetInput(input_->GetInput(), input_->GetInputCommand());
    // シーンにリソース管理機能を設定
    IScene::SetResourceManager(resource_->GetTextureManager(), resource_->GetModelManager(), resource_->GetAnimationManager(), scene_->GetGameObjectManager());
    // シーンに描画機能を設定
    IScene::SetRender(graphics_->GetRenderPassCtrl(), graphics_->GetRenderQueue());
    // デバック描画機能を設定
    IScene::SetDebug(graphics_->GetDebugRenderer());
}

void Engine::MainLoop() {
    while (!core_->IsWindowCloseRequested()) {
        PreUpdate();

        if (isActiveUpdate_ && !isPause_) {
            scene_->UpdateGameplay();
        } else {
            scene_->UpdateDebug();
        }

        scene_->GetCollisionManager()->DebugDraw(graphics_->GetDebugRenderer());

        PostUpdate();

        PreDraw();
        scene_->Draw();
        graphics_->GetRenderQueue()->Execute();
        // ポストエフェクトを実行
        graphics_->GetPostEffectManager()->Execute();
        PostDraw();
    }
}

void Engine::PreUpdate() {
    // デバイスチェック
    graphics_->GetGraphicsDevice()->CheckDeviceStatus();
    core_->Update(); // FPS
    input_->Update(); // 入力処理

    graphics_->GetImGuiManager()->BeginFrame();
    graphics_->GetRenderQueue()->Begin();

#ifdef USE_IMGUI
    graphics_->GetDebugRenderer()->Clear();
    editor_->Update();

    isActiveUpdate_ = editor_->IsActiveUpdate();
    isPause_ = editor_->IsPause();

    // 更新停止中にリセットが必要なら実行
    if (isActiveUpdate_) {
        isReset_ = false;
    } else if (!isReset_) {
        scene_->ResetCurrentScene();
        isReset_ = true;
    }

    // シーン切り替えリクエストを処理
    auto* request = scene_->GetSceneChangeRequest();
    if (request->HasChangeRequest()) {
        scene_->ChangeScene(request->GetRequestScene());
        request->ClearChangeRequest();
        request->SetCurrentSceneName(scene_->GetCurrentSceneName());
    }

    scene_->GetSceneManager()->DebugUpdate();
    scene_->GetGameObjectManager()->DebugUpdateAll();
#endif
}

void Engine::PostUpdate() {
    graphics_->GetDebugRenderer()->Update();
    graphics_->GetImGuiManager()->EndFrame();
}

void Engine::PreDraw() { 
    graphics_->BeginFrame(); 
}

void Engine::PostDraw() { 
    graphics_->EndFrame();
}

void Engine::Finalize() {
    subsystemRegistry_.FinalizeAll();
}

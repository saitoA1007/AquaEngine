#include "GraphicsSubsystem.h"
#include "GpuResource.h"
#include "SrvResource.h"
#include "Sprite.h"
#include "SpriteRenderer.h"
#include "ModelRenderer.h"
#include "CoreSubsystem.h"
using namespace GameEngine;

void GraphicsSubsystem::Initialize() {
    auto* windowsApp = context_.core->GetWindowsApp();

    // DirectXの機能を生成
    graphicsDevice_ = std::make_unique<GraphicsDevice>();
    graphicsDevice_->Initialize(
        windowsApp->GetHwnd(),
        windowsApp->kWindowWidth,
        windowsApp->kWindowHeight);

    // dxcCompilerの初期化
    dxc_ = std::make_unique<DXC>();
    dxc_->Initialize();

    // PSO作成
    InitializePSO();

    // PSO管理機能の作成
    psoManager_ = std::make_unique<PSOManager>();
    psoManager_->Initialize(graphicsDevice_->GetDevice(), dxc_.get());
    psoManager_->DefaultLoadPSO();
    psoManager_->DeaultLoadPostEffectPSO();

    // GPUリソースの静的初期化
    GpuResource::StaticInitialize(graphicsDevice_->GetDevice());
    SrvResource::StaticInitialize(graphicsDevice_->GetSrvManager());

    // レンダーテクスチャ機能を生成
    renderTextureManager_ = std::make_unique<RenderTextureManager>();
    renderTextureManager_->Initialize(graphicsDevice_->GetRtvManager(), graphicsDevice_->GetDsvManager(), graphicsDevice_->GetDevice());

    // レンダーパスの管理機能
    renderPassController_ = std::make_unique<RenderPassController>();
    renderPassController_->Initialize(renderTextureManager_.get(), graphicsDevice_->GetCommandList());

    // 描画コマンド管理
    renderQueue_ = std::make_unique<RenderQueue>();
    renderQueue_->Initialize(graphicsDevice_->GetCommandList(), psoManager_.get(), renderPassController_.get());

    // ポストエフェクトマネージャーの初期化
    postEffectManager_ = std::make_unique<PostEffectManager>();
    postEffectManager_->Initialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager(), psoManager_.get(), renderPassController_.get());

    // 描画の流れを管理するクラスを初期化
    renderPipeline_ = std::make_unique<RenderPipeline>();
    renderPipeline_->Initialize(graphicsDevice_.get(), postEffectManager_.get(), renderPassController_.get());
    renderPipeline_->SetCopyPSO(copyPSO_.get());

    // ImGuiの初期化
    imGuiManager_ = std::make_unique<ImGuiManager>();
    imGuiManager_->Initialize(graphicsDevice_->GetDevice(), graphicsDevice_->GetCommandList(), graphicsDevice_->GetSwapChainDesc(),
        windowsApp, graphicsDevice_->GetSrvManager());

    // デバックレンダラー
    debugRenderer_ = std::make_unique<DebugRenderer>();

    // Rendererの静的初期化
    Sprite::StaticInitialize(windowsApp->kWindowWidth, windowsApp->kWindowHeight);
    SpriteRenderer::StaticInitialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager());
    ModelRenderer::StaticInitialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager());
}

void GraphicsSubsystem::InitializePSO() {
    // CopyPSOの初期化
    copyPSO_ = std::make_unique<CopyPSO>();
    copyPSO_->Initialize(graphicsDevice_->GetDevice(), L"Resources/Shaders/PostEffect/FullScreen.VS.hlsl", L"Resources/Shaders/PostEffect/Copy.PS.hlsl", dxc_.get());

    /// PostProcessのPSOを初期化

    // BloomPSOの初期化
    bloomPSO_ = std::make_unique<BloomPSO>();
    bloomPSO_->Initialize(graphicsDevice_->GetDevice(), L"Resources/Shaders/PostEffect/Bloom.VS.hlsl", dxc_.get(),
        L"Resources/Shaders/PostEffect/HighLumMask.PS.hlsl",
        L"Resources/Shaders/PostEffect/Bloom.PS.hlsl",
        L"Resources/Shaders/PostEffect/BloomResult.PS.hlsl",
        L"Resources/Shaders/PostEffect/BloomComposite.hlsl");
}

void GraphicsSubsystem::Finalize() {
    imGuiManager_->Finalize();
}

void GraphicsSubsystem::BeginFrame() {
    renderPipeline_->BeginFrame();
}

void GraphicsSubsystem::EndFrame() {
    renderPipeline_->EndFrame(imGuiManager_.get());
}
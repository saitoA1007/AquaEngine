#include "GraphicsSubsystem.h"
#include "GpuResource.h"
#include "SrvResource.h"
#include "BufferRefResource.h"
#include "Sprite.h"
#include "SpriteRenderer.h"
#include "ModelRenderer.h"
#include "CoreSubsystem.h"
#include "Animator.h"
#include "ParticleBehaviorGPU.h"
#include "Debug/PixCapture.h"
using namespace GameEngine;

void GraphicsSubsystem::Initialize() {
    auto* windowsApp = context_.core->GetWindowsApp();

    // PIXのGPUキャプチャ機能を準備する。
    // WinPixGpuCapturer.dll は ID3D12Device の生成より「前」にロードしないと効かないため、
    // 必ずこの位置で呼ぶこと。
    PixCapture::GetInstance().Initialize();

    // DirectXの機能を生成
    graphicsDevice_ = std::make_unique<GraphicsDevice>();
    graphicsDevice_->Initialize(
        windowsApp->GetHwnd(),
        windowsApp->kWindowWidth,
        windowsApp->kWindowHeight);

    // HUD表示の設定は、スワップチェーン生成後に行う。
    // （生成前に呼ぶと左上のオーバーレイ設定が反映されない）
    PixCapture::GetInstance().SetHudVisible(false);

    // PIXSetTargetWindow を設定すると、そのHWNDのPresent以外ではキャプチャが
    // 開始/終了されなくなる。HWNDが想定と違うとキャプチャが一切取れなくなるため、
    // 既定では設定しない。複数ウィンドウで対象を絞りたい場合だけ有効化する。
    //PixCapture::GetInstance().SetTargetWindow(windowsApp->GetHwnd());

    // dxcCompilerの初期化
    dxc_ = std::make_unique<DXC>();
    dxc_->Initialize();

    // PSO作成
    InitializePSO();

    // PSO管理機能の作成
    psoManager_ = std::make_unique<PSOManager>();
    psoManager_->Initialize(graphicsDevice_->GetDevice(), dxc_.get());
    psoManager_->DefaultLoadPSO();
    psoManager_->DefaultLoadPostEffectPSO();

    // GPUリソースの静的初期化
    GpuResource::StaticInitialize(graphicsDevice_->GetDevice());
    SrvResource::StaticInitialize(graphicsDevice_->GetSrvManager());

    // バッファのアクセスデータ管理機能の初期化
    bufferRefManager_ = std::make_unique<BufferRefManager>();
    bufferRefManager_->Initialize();

    // アクセスデータ管理機能を登録する
    BufferRefResource::StaticInitialize(bufferRefManager_.get(), graphicsDevice_->GetSrvManager()->GetStartSrvIndex(SrvHeapType::Buffer));

    // レンダーテクスチャ機能を生成
    renderTextureManager_ = std::make_unique<RenderTextureManager>();
    renderTextureManager_->Initialize(graphicsDevice_->GetRtvManager(), graphicsDevice_->GetDsvManager(), graphicsDevice_->GetDevice());

    // レンダーパスの管理機能
    renderPassController_ = std::make_unique<RenderPassController>();
    renderPassController_->Initialize(renderTextureManager_.get(), graphicsDevice_->GetCommandList());

    // レイトレーシング用のパイプライン
    raytracingPipeline_ = std::make_unique<RaytracingPipeline>();
    raytracingPipeline_->Initialize(graphicsDevice_->GetDevice(), graphicsDevice_->GetSrvManager(), dxc_.get());

    // 描画コマンド発行機能
    renderQueue_ = std::make_unique<RenderQueue>();
    renderQueue_->Initialize();

    // シーン描画の管理
    sceneRenderManager_ = std::make_unique<SceneRenderManager>();
    sceneRenderManager_->Initialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager(), psoManager_.get(),
        renderPassController_.get(), raytracingPipeline_.get(), bufferRefManager_.get(),renderQueue_.get());

    // ポストエフェクトマネージャーの初期化
    postEffectManager_ = std::make_unique<PostEffectManager>();
    postEffectManager_->Initialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager(), psoManager_.get(), renderPassController_.get());

    // 描画の流れを管理するクラスを初期化
    renderPipeline_ = std::make_unique<RenderPipeline>();
    renderPipeline_->Initialize(graphicsDevice_.get(), sceneRenderManager_.get(), postEffectManager_.get(), renderPassController_.get());
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
    // アニメーション用の静的初期化
    Animator::StaticInitialize(graphicsDevice_->GetCommandList(), psoManager_.get());
    // Csパーティクル用
    ParticleBehaviorGPU::StaticInitialize(graphicsDevice_->GetCommandList(), psoManager_.get());
}

void GraphicsSubsystem::InitializePSO() {
    // CopyPSOの初期化
    copyPSO_ = std::make_unique<CopyPSO>();
    copyPSO_->Initialize(graphicsDevice_->GetDevice(), L"Resources/Shaders/PostEffect/FullScreen.VS.hlsl", L"Resources/Shaders/PostEffect/Copy.PS.hlsl", dxc_.get());
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
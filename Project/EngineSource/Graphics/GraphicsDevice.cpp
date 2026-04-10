#include"GraphicsDevice.h"
#include "LogManager.h"

using namespace GameEngine;

void GraphicsDevice::Initialize(HWND hwnd, uint32_t width, uint32_t height) {
    // 初期化を開始するログ
    LogManager::GetInstance().Log("GraphicsDevice Class start Initialize");

    // デバイスを生成
    device_ = std::make_unique<DXDevice>();
    device_->Initialize();

#ifdef _DEBUG
    debugger_ = std::make_unique<DXDebugger>();
    debugger_->InitializeDebugLayer(device_->GetDevice());
#endif

    // コマンドの初期化
    command_ = std::make_unique<DXCommand>();
    command_->Initialize(device_->GetDevice());

    // スワップチェーンの作成
    swapChain_ = std::make_unique<DXSwapChain>();
    swapChain_->Initialize(hwnd, width, height, device_->GetFactory(), command_->GetQueue());

    // SRVを生成する
    srvManager_ = std::make_unique<SrvManager>();
    srvManager_->Initialize(device_->GetDevice());

    // RTVシステムを生成
    rtvManager_ = std::make_unique<RtvManager>();
    rtvManager_->Initialize(device_->GetDevice());

    // dsvを生成する
    dsvManager_ = std::make_unique<DsvManager>();
    dsvManager_->Initialize(device_->GetDevice());

    // RTV、DSVの生成
    renderTarget_ = std::make_unique<DXRenderTarget>();
    renderTarget_->Initialize(device_->GetDevice(), swapChain_->GetSwapChain());

    // 深度ステンシルの初期化
    depthStencil_ = std::make_unique<DXDepthStencil>();
    depthStencil_->Initialize(device_->GetDevice(), renderTarget_->GetDSVHeap(), width, height, srvManager_.get());

    // フェンスの生成
    fence_ = std::make_unique<DXFence>();
    fence_->Initialize(device_->GetDevice());

    // ビューポート、シザー矩形を生成
    viewportState_ = std::make_unique<DXViewportState>();
    viewportState_->Initialize(width, height);

    // 初期化を終了するログ
    LogManager::GetInstance().Log("GraphicsDevice Class end Initialize\n");
}

void GraphicsDevice::CloseCommandList() {
    command_->Close();
}

void GraphicsDevice::ExecuteCommand() {
    command_->Execute();
}

void GraphicsDevice::ResetCommandList() {
    command_->Reset();
}

void GraphicsDevice::WaitForGPU() {
    fence_->WaitForGPU(command_->GetQueue());
}

void GraphicsDevice::Present() {
    swapChain_->Present();
}
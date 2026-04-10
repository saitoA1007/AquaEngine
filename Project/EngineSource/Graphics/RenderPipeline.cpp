#include "RenderPipeline.h"
#include "ImGuiManager.h"
#include "LogManager.h"

using namespace GameEngine;

void RenderPipeline::Initialize(GraphicsDevice* graphicsDevice, PostEffectManager* postEffectManager, RenderPassController* renderPassController) {
    LogManager::GetInstance().Log("RenderPipeline start Initialize");

    // ポストエフェクトの管理
    postEffectManager;

    renderPassController_ = renderPassController;

    // DirectXのコア機能を取得
    graphicsDevice_ = graphicsDevice;

    // FPS固定初期化
    frameRateController_ = std::make_unique<FrameRateController>();
    frameRateController_->InitializeFixFPS();

    LogManager::GetInstance().Log("RenderPipeline end Initialize\n");
}

void RenderPipeline::BeginFrame() {
      // ヒープを設定する
    ID3D12DescriptorHeap* descriptorHeaps[] = { graphicsDevice_->GetSrvManager()->GetSRVHeap() };
    graphicsDevice_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
}

void RenderPipeline::EndFrame(ImGuiManager* imGuiManager) {

    /// 最終結果を描画する
    // バックバッファをレンダーターゲットに遷移
    TransitionBackBuffer(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // バックバッファを描画先に設定
    uint32_t backBufferIndex = graphicsDevice_->GetBackBufferIndex();
    auto rtvHandle = graphicsDevice_->GetSwapChainRTVHandle(backBufferIndex);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = graphicsDevice_->GetDSVHeap()->GetCPUDescriptorHandleForHeapStart();
    graphicsDevice_->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

    // ビューポート、シザーを設定
    graphicsDevice_->GetCommandList()->RSSetViewports(1, &graphicsDevice_->GetViewport());
    graphicsDevice_->GetCommandList()->RSSetScissorRects(1, &graphicsDevice_->GetScissorRect());

#ifdef USE_IMGUI
    // ImGuiを描画
    imGuiManager->Draw();
#else
    // ポストプロセス結果を描画
    copyPSO_->Draw(graphicsDevice_->GetCommandList(), renderPassController_->GetFinalOutputSRV());
#endif

    // バックバッファをPresentに遷移
    TransitionBackBuffer(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    // コマンドリストの内容を確定させる。すべてのコマンドを積んでからcloseにすること
    graphicsDevice_->CloseCommandList();
    // GPUにコマンドリストの実行を行わせる
    graphicsDevice_->ExecuteCommand();

    // GPUとOSに画面の交換を行うように通知する
    graphicsDevice_->Present();

    // GPUを待つ
    graphicsDevice_->WaitForGPU();

    // 次のフレーム用にコマンドリストを準備
    graphicsDevice_->ResetCommandList();

    // FPS固定
    frameRateController_->UpdateFixFPS();
}

void RenderPipeline::TransitionBackBuffer(D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
    uint32_t backBufferIndex = graphicsDevice_->GetBackBufferIndex();
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = graphicsDevice_->GetSwapChainResource(backBufferIndex);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;
    graphicsDevice_->GetCommandList()->ResourceBarrier(1, &barrier);
}
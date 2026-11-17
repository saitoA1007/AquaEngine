#pragma once
#include "DXDevice.h"
#include "DXCommand.h"
#include "DXSwapChain.h"
#include "DXRenderTarget.h"
#include "DXDepthStencil.h"
#include "DXFence.h"
#include "DXDebugger.h"
#include "DXViewportState.h"
#include "FrameContext.h"
#include "FrameUploader.h"
#include "DynamicBufferRegistry.h"

#include "RtvManager.h"
#include "SrvManager.h"
#include "DsvManager.h"

namespace GameEngine {

    class GraphicsDevice {
    public:
        GraphicsDevice() = default;
        ~GraphicsDevice();

        /// <summary>
        /// 初期化
        /// </summary>
        /// <param name="hwnd"></param>
        /// <param name="width"></param>
        /// <param name="height"></param>
        /// <param name="srvManager"></param>
        void Initialize(HWND hwnd, uint32_t width, uint32_t height);

        /// <summary>
        /// 毎フレームデバイスの状態を確認する
        /// </summary>
        void CheckDeviceStatus();

        /// <summary>
        /// コマンドリストを閉じる
        /// </summary>
        void CloseCommandList();

        /// <summary>
        /// コマンドを実行
        /// </summary>
        /// <param name="preCommandList">メインより先に実行するアップロード用コマンドリスト。無ければnullptr</param>
        void ExecuteCommand(ID3D12CommandList* preCommandList = nullptr);

        /// <summary>
        /// 現在のフレームの終わりをSignalし、次のフレームスロットへ進む。
        /// 次のスロットを前回使ったフレームのGPU実行完了のみを待つ
        /// </summary>
        void MoveToNextFrame();

        /// <summary>
        /// 現在のフレームスロットのアロケータでコマンドリストをリセット
        /// </summary>
        void ResetCommandList();

        /// <summary>
        /// GPUの全処理の完了を待機。終了処理やリソースの作り直しで使う
        /// </summary>
        void WaitForGPU();

        /// <summary>
        /// 画面を提示(Present)
        /// </summary>
        void Present();

        /// <summary>
        /// このフレームでCPUが書き込んだDynamicBufferをGPU側へ転送するコマンドを記録する
        /// </summary>
        /// <returns>メインより先に実行するコマンドリスト。転送するものが無ければnullptr</returns>
        ID3D12CommandList* RecordUploads();

    public:
        ID3D12Device5* GetDevice() const { return device_->GetDevice(); }

        ID3D12GraphicsCommandList4* GetCommandList() const { return command_->GetCommandList(); }

        IDXGISwapChain4* GetSwapChain() const { return swapChain_->GetSwapChain(); }
        DXGI_SWAP_CHAIN_DESC1 GetSwapChainDesc() const { return swapChain_->GetSwapChainDesc(); }
        uint32_t GetBackBufferIndex() const { return swapChain_->GetBackBufferIndex(); }

        ID3D12DescriptorHeap* GetRTVHeap() const { return renderTarget_->GetRTVHeap(); }
        ID3D12DescriptorHeap* GetDSVHeap() const { return renderTarget_->GetDSVHeap(); }

        uint32_t GetRTVDescriptorSize() const { return renderTarget_->GetRTVDescriptorSize(); }
        uint32_t GetDSVDescriptorSize() const { return renderTarget_->GetDSVDescriptorSize(); }
        D3D12_CPU_DESCRIPTOR_HANDLE GetSwapChainRTVHandle(const uint32_t& index) const { return renderTarget_->GetSwapChainRTVHandle(index); }
        ID3D12Resource* GetSwapChainResource(const uint32_t& index) const { return renderTarget_->GetSwapChainResource(index); }

        const D3D12_VIEWPORT& GetViewport() const { return viewportState_->GetViewport(); }
        const D3D12_RECT& GetScissorRect() const { return viewportState_->GetScissorRect(); }

        ID3D12Resource* GetDepthStencilResource()const { return depthStencil_->GetResource(); }
        CD3DX12_GPU_DESCRIPTOR_HANDLE GetDepthStencilSRVHandle() const { return depthStencil_->GetSRVHandle(); }

        RtvManager* GetRtvManager() { return rtvManager_.get(); }

        SrvManager* GetSrvManager() { return srvManager_.get(); }

        DsvManager* GetDsvManager() { return dsvManager_.get(); }

        // フレームごとのアップロード領域
        FrameUploader* GetFrameUploader() { return frameUploader_.get(); }

        // DynamicBufferの登録先。所有権は渡さない
        std::weak_ptr<DynamicBufferRegistry> GetDynamicBufferRegistry() const { return dynamicBufferRegistry_; }

    private:
        GraphicsDevice(const GraphicsDevice&) = delete;
        GraphicsDevice& operator=(const GraphicsDevice&) = delete;

        std::unique_ptr<DXDevice> device_;
        std::unique_ptr<DXCommand> command_;
        std::unique_ptr<DXSwapChain> swapChain_;
        std::unique_ptr<DXRenderTarget> renderTarget_;
        std::unique_ptr<DXDepthStencil> depthStencil_;
        std::unique_ptr<DXFence> fence_;
        std::unique_ptr<DXViewportState> viewportState_;

        // 各フレームスロットの終わりにSignalしたフェンス値
        uint64_t frameFenceValues_[kFrameCount] = {};

        // rtvを管理する機能
        std::unique_ptr<RtvManager> rtvManager_;

        // srvメモリを管理する機能
        std::unique_ptr<SrvManager> srvManager_;

        // dsvを管理する機能
        std::unique_ptr<DsvManager> dsvManager_;

        // フレームごとのアップロード領域
        std::unique_ptr<FrameUploader> frameUploader_;

        // DynamicBufferの登録先
        std::shared_ptr<DynamicBufferRegistry> dynamicBufferRegistry_;

#ifdef _DEBUG
        std::unique_ptr<DXDebugger> debugger_;
#endif
    };
}


#pragma once
#include <d3d12.h>
#include <string>
#include <vector>
#include "Externals/DirectXTex/d3dx12.h"
#include "RenderTexture.h"

namespace GameEngine {

    class RenderPass {
    public:

        RenderPass(const std::string& name, ID3D12GraphicsCommandList* commandList, RenderTexture* renderTexture);

        // 描画前処理
        void PrePass();

        // 描画後処理
        void PostPass();

        // RTV描画後にUAVとして使用するためUAV状態へ遷移する
        void SwitchToUnorderedAccess();

        // UAV書き込み後にGPU側の書き込みを完了させるバリアを挿入する
        void InsertUavBarrier();

        // srvIndexを取得
        uint32_t GetSrvIndex() { return renderTexture_->GetSrvIndex(); }

        CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvHandle();

        // 名前を取得
        const std::string GetName() const { return name_; }

    private:

        RenderTexture* renderTexture_ = nullptr;

        ID3D12GraphicsCommandList* commandList_ = nullptr;
        D3D12_VIEWPORT viewport_{};
        D3D12_RECT scissorRect_{};

        // パスの名前
        std::string name_;

        // レンダーモード
        RenderTextureMode mode_;
    };
}
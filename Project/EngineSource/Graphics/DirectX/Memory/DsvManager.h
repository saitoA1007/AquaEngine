#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <deque>
#include "Externals/DirectXTex/d3dx12.h"

namespace GameEngine {

    struct DsvContext {
        uint32_t width;
        uint32_t height;
        DXGI_FORMAT format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 一般的な24bit深度+8bitステンシル
        float depthClearValue = 1.0f;                      // 深度の初期値
        uint8_t stencilClearValue = 0;                     // ステンシルの初期値
    };

    class DsvManager {
    public:
        DsvManager() = default;
        ~DsvManager() = default;

        void Initialize(ID3D12Device* device);

        /// <summary>
        /// 深度ステンシルリソースを作成
        /// </summary>
        uint32_t CreateDepthStencilResource(const DsvContext& context);

        // 深度ステンシルリソースを作成。srvに対応
        // この関数を使用して作成したリソースを解放したい時、srvを側も解放する機能はまだ作成されていない。
        uint32_t CreateDepthStencilResource(const DsvContext& context, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);

        /// <summary>
        /// リソースの解放とインデックスの回収
        /// </summary>
        void ReleaseIndex(uint32_t index);

        ID3D12Resource* GetResource(uint32_t index) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const;

    private:
        DsvManager(const DsvManager&) = delete;
        DsvManager& operator=(const DsvManager&) = delete;

        ID3D12Device* device_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;

        uint32_t maxDsvCount_ = 10;
        uint32_t descriptorSizeDSV_ = 0;
        uint32_t nextDsvIndex_ = 0;
        std::deque<uint32_t> freeIndices_;

        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources_;
    };
}

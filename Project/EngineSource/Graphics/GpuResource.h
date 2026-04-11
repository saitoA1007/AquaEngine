#pragma once
#include <wrl.h>
#include <d3d12.h>

namespace GameEngine {

    /// <summary>
    /// リソースの元となる汎用的な基底クラス
    /// </summary>
    class GpuResource {
    public:
        virtual ~GpuResource() = default;

        // リソースを取得
        ID3D12Resource* GetResource() const { return resource_.Get(); }

        // アドレスを取得
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return resource_->GetGPUVirtualAddress();}

    protected:
        // リソース
        Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    };
}

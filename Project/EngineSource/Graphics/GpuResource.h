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

        // リソースのゲッター
        ID3D12Resource* GetResource() const { return resource_.Get(); }

    protected:
        // リソース
        Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    };
}

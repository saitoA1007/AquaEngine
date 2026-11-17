#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include "ResourceGarbageCollector.h"

namespace GameEngine {

    // 前方宣言
    class DynamicBuffer;
    class DynamicBufferRegistry;

    /// <summary>
    /// リソースの元となる汎用的な基底クラス
    /// </summary>
    class GpuResource {
    public:
        virtual ~GpuResource() {
            // GPUが前のフレームで参照している可能性があるため、実行完了後に破棄する
            if (resource_) {
                ResourceGarbageCollector::GetInstance().Add(std::move(resource_));
            }
        }

        /// <summary>
        /// 静的初期化
        /// </summary>
        /// <param name="device">デバイス</param>
        /// <param name="dynamicBufferRegistry">CPUが書き込むバッファの転送先</param>
        static void StaticInitialize(ID3D12Device5* device, std::weak_ptr<DynamicBufferRegistry> dynamicBufferRegistry) {
            device_ = device;
            dynamicBufferRegistry_ = std::move(dynamicBufferRegistry);
        }

        // リソースを取得
        ID3D12Resource* GetResource() const { return resource_.Get(); }

        // アドレスを取得
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return resource_->GetGPUVirtualAddress();}

    protected:
        // CPUが書き込み、フレーム末尾にGPUへ転送されるバッファを作成する
        static std::shared_ptr<DynamicBuffer> CreateDynamicBuffer(size_t sizeInBytes);

    protected:
        static ID3D12Device5* device_;
        static std::weak_ptr<DynamicBufferRegistry> dynamicBufferRegistry_;
        // リソース
        Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    };
}

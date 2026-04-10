#pragma once
#include <vector>
#include <string>
#include <d3d12.h>
#include <wrl.h>
#include "Externals/DirectXTex/d3dx12.h"

struct RenderTarget {
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandle{};
    uint32_t width = 0;
    uint32_t height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    std::string name;
};

// エフェクトの実行コンテキスト
struct EffectContext {
    ID3D12GraphicsCommandList* commandList = nullptr;
    D3D12_VIEWPORT viewport{};
    D3D12_RECT scissorRect{};
    ID3D12DescriptorHeap* srvHeap = nullptr;

    // 入力リソース（前段のエフェクト出力）
    D3D12_GPU_DESCRIPTOR_HANDLE inputSRV{};
    // 深度バッファなどの追加入力
    D3D12_GPU_DESCRIPTOR_HANDLE depthSRV{};

    // 出力先
    D3D12_CPU_DESCRIPTOR_HANDLE outputRTV{};
};

class IPostEffect {
public:

    virtual ~IPostEffect() = default;

    // エフェクト名を取得
    virtual const char* GetName() const = 0;

    // 初期化
    virtual void Initialize(
        ID3D12Device* device,
        uint32_t width,
        uint32_t height
    ) = 0;

    // エフェクトを実行（入力RTを受け取り、出力RTに描画）
    virtual void Execute(
        const EffectContext& context,
        const RenderTarget& input,
        const RenderTarget& output
    ) = 0;

    // 有効/無効切り替え
    virtual void SetEnabled(bool enabled) { enabled_ = enabled; }
    virtual bool IsEnabled() const { return enabled_; }

    // リサイズ対応
    virtual void OnResize(uint32_t width, uint32_t height) {}

    // ImGui設定UI（オプション）
    virtual void RenderUI() {}

protected:
    bool enabled_ = true;

};
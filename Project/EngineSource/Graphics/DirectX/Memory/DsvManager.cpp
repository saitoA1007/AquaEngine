#include"DsvManager.h"
#include"DepthStencilTexture.h"
using namespace GameEngine;

#include "DescriptorHeap.h"
#include "DescriptorHandle.h"
#include <cassert>

using namespace GameEngine;

void DsvManager::Initialize(ID3D12Device* device) {
    device_ = device;

    // DSV用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
    dsvHeap_ = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, maxDsvCount_, false);

    descriptorSizeDSV_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    resources_.reserve(maxDsvCount_);
}

uint32_t DsvManager::CreateDepthStencilResource(const DsvContext& context) {
    uint32_t index;
    if (!freeIndices_.empty()) {
        index = freeIndices_.front();
        freeIndices_.pop_front();
    } else {
        assert(nextDsvIndex_ < maxDsvCount_ && "DSV heap is full");
        index = nextDsvIndex_++;
    }

    // リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;

    // DepthStencilTextureをウィンドウのサイズで作成DXGI_FORMAT_D24_UNORM_S8_UINT
    resource = CreateDepthStencilTextureResource(device_, context.width, context.height);

    // DSVの設定
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    // ハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandle(dsvHeap_.Get(), descriptorSizeDSV_, index);

    // DSVHeapの先頭にDSVを作る
    device_->CreateDepthStencilView(resource.Get(), &dsvDesc, handle);

    // リソースを登録
    resources_.push_back(std::move(resource));

    return index;
}

uint32_t DsvManager::CreateDepthStencilResource(const DsvContext& context, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle) {
    uint32_t index;
    if (!freeIndices_.empty()) {
        index = freeIndices_.front();
        freeIndices_.pop_front();
    } else {
        assert(nextDsvIndex_ < maxDsvCount_ && "DSV heap is full");
        index = nextDsvIndex_++;
    }

    // リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;

    // DepthStencilTextureをウィンドウのサイズで作成DXGI_FORMAT_D24_UNORM_S8_UINT
    resource = CreateDepthStencilTextureResource(device_, context.width, context.height);

    // DSVの設定
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    // ハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle = GetCPUDescriptorHandle(dsvHeap_.Get(), descriptorSizeDSV_, index);

    // DSVHeapの先頭にDSVを作る
    device_->CreateDepthStencilView(resource.Get(), &dsvDesc, cpuhandle);

    // 深度を読み込める用にsrvを作成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;// 2Dテクスチャ
    srvDesc.Texture2D.MipLevels = 1;

   // オブジェクト描画用SRV
    device_->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);

    // リソースを登録
    resources_.push_back(std::move(resource));

    return index;
}

void DsvManager::ReleaseIndex(uint32_t index) {
    assert(index < maxDsvCount_);
    if (resources_[index]) {
        resources_[index].Reset();
        freeIndices_.push_back(index);
    }
}

ID3D12Resource* DsvManager::GetResource(uint32_t index) const {
    return resources_[index].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DsvManager::GetCPUHandle(uint32_t index) const {
    return GetCPUDescriptorHandle(dsvHeap_.Get(), descriptorSizeDSV_, index);
}
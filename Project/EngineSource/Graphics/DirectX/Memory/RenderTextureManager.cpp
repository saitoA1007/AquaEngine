#include"RenderTextureManager.h"

using namespace GameEngine;

void RenderTextureManager::Initialize(RtvManager* rtvManager, SrvManager* srvManager, DsvManager* dsvmanager, ID3D12Device* device) {
	rtvManager_ = rtvManager;
	srvManager_ = srvManager;
    dsvmanager_ = dsvmanager;
    device_ = device;
}

void RenderTextureManager::Create(const std::string& name, RenderTextureMode mode, RtvContext context) {

    // テクスチャ情報を作成
    RenderTextureContext renderTextureContext{};
    renderTextureContext.width = context.width;
    renderTextureContext.height = context.height;
    renderTextureContext.mode = mode;

    // RTVの作成
    if (mode != RenderTextureMode::DsvOnly) {
        uint32_t rtvIndex = rtvManager_->CreateRenderTargetResource(context);
        renderTextureContext.rtvIndex = rtvIndex;
        renderTextureContext.rtvHandle = rtvManager_->GetCPUHandle(rtvIndex);
        renderTextureContext.resource = rtvManager_->GetResource(rtvIndex);

        // RTV用のSRVを作成
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = context.format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;

        // SRVハンドル取得
        uint32_t srvIndex = srvManager_->AllocateSrvIndex(SrvHeapType::System);
        D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle = srvManager_->GetCPUHandle(srvIndex);
        CD3DX12_GPU_DESCRIPTOR_HANDLE srvGPUHandle = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex));
        
        renderTextureContext.srvIndex = srvIndex;
        renderTextureContext.srvGpuHandle = srvGPUHandle;

        // オブジェクト描画用SRV
        device_->CreateShaderResourceView(renderTextureContext.resource, &srvDesc, srvCPUHandle);
    }

    // DSVの作成
    if (mode != RenderTextureMode::RtvOnly) {
        DsvContext dsvContext;
        dsvContext.width = context.width;
        dsvContext.height = context.height;

        renderTextureContext.dsvHaveSrvIndex = srvManager_->AllocateSrvIndex(SrvHeapType::System);
        D3D12_CPU_DESCRIPTOR_HANDLE dsvhavesrvCPUHandle = srvManager_->GetCPUHandle(renderTextureContext.dsvHaveSrvIndex);
        CD3DX12_GPU_DESCRIPTOR_HANDLE srvGPUHandle = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(renderTextureContext.dsvHaveSrvIndex));

        // dsvのみならメインのSRVインデックスに割り当てる
        if (mode == RenderTextureMode::DsvOnly) {
            renderTextureContext.srvIndex = renderTextureContext.dsvHaveSrvIndex;
            renderTextureContext.srvGpuHandle = srvGPUHandle;
        }

        renderTextureContext.dsvIndex = dsvmanager_->CreateDepthStencilResource(dsvContext, dsvhavesrvCPUHandle);
        renderTextureContext.dsvResource = dsvmanager_->GetResource(renderTextureContext.dsvIndex);
        renderTextureContext.dsvHandle = dsvmanager_->GetCPUHandle(renderTextureContext.dsvIndex);
    }

    // RenderTextureを作成
    std::unique_ptr<RenderTexture> renderTexture = std::make_unique<RenderTexture>();
    renderTexture->Initialize(renderTextureContext);

    // 登録する
    renderTextures_[name] = std::move(renderTexture);
}

RenderTexture* RenderTextureManager::GetRenderTexture(const std::string& name) {
    auto it = renderTextures_.find(name);
    if (it != renderTextures_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void RenderTextureManager::Release(const std::string& name) {
    auto it = renderTextures_.find(name);
    if (it != renderTextures_.end()) {
        // RtvとSrvのインデックスを解放
        uint32_t rtvIndex = it->second->GetRtvIndex();
        uint32_t srvIndex = it->second->GetSrvIndex();

        rtvManager_->ReleseIndex(rtvIndex);
        srvManager_->ReleseIndex(srvIndex);

        // マップから削除
        renderTextures_.erase(it);
    }
}
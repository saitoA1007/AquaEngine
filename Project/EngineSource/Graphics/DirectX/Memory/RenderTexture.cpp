#include "RenderTexture.h"
#include <cassert>
#include "DepthStencilTexture.h"
using namespace GameEngine;

RtvManager* RenderTexture::rtvManager_ = nullptr;
DsvManager* RenderTexture::dsvManager_ = nullptr;

RenderTexture::~RenderTexture() {
	// カラーRTのデスクリプタ解放
	if (mode_ != RenderTextureMode::DsvOnly) {
		if (rtvIndex_ != 0) {
			rtvManager_->ReleseIndex(rtvIndex_);
		}
		// DsvOnlyでない場合のSRVインデックスを解放
		if (srvIndex_ != 0) {
			srvManager_->ReleseIndex(srvIndex_);
		}
	}

	// 深度RTのデスクリプタ解放
	if (mode_ != RenderTextureMode::RtvOnly) {
		if (dsvIndex_ != 0) {
			dsvManager_->ReleseIndex(dsvIndex_);
		}
		// RtvAndDsv: 深度用 SRV インデックスは srvIndex_ と別
		// DsvOnly  : srvIndex_ == dsvSrvIndex_ なので既に上で解放済み
		if (mode_ == RenderTextureMode::RtvAndDsv && dsvSrvIndex_ != 0) {
			srvManager_->ReleseIndex(dsvSrvIndex_);
		}
		// DsvOnly の場合は srvIndex_ に dsvSrvIndex_ を代入しているため
		// srvIndex_ の解放として上のブロックで処理済み
		if (mode_ == RenderTextureMode::DsvOnly && srvIndex_ != 0) {
			srvManager_->ReleseIndex(srvIndex_);
		}
	}
}

void RenderTexture::Create(uint32_t width, uint32_t height, RenderTextureMode mode, DXGI_FORMAT colorFormat) {
	width_ = width;
	height_ = height;
	mode_ = mode;

	if (mode_ != RenderTextureMode::DsvOnly) {
		CreateColorTarget(width, height, colorFormat);
	}

	if (mode_ != RenderTextureMode::RtvOnly) {
		CreateDepthTarget(width, height);

		// DsvOnlyの場合、深度SRVをメインのSRVとして扱う
		if (mode_ == RenderTextureMode::DsvOnly) {
			srvIndex_ = dsvSrvIndex_;
			srvGpuHandle_ = depthSrvGpuHandle_;
		}
	}
}

void RenderTexture::TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList) {
	if (isTarget_) {
		return;
	}

	// 描画モードに遷移
	isTarget_ = true;

	// RTVの遷移
	if (mode_ != RenderTextureMode::DsvOnly && resource_) {
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = resource_.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);
	}

	// DSVの遷移
	if (mode_ != RenderTextureMode::RtvOnly && depthResource_) {
		D3D12_RESOURCE_BARRIER depthbarrier{};
		depthbarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthbarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthbarrier.Transition.pResource = depthResource_.Get();
		depthbarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		depthbarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		depthbarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		commandList->ResourceBarrier(1, &depthbarrier);
	}
}

void RenderTexture::TransitionToShaderResource(ID3D12GraphicsCommandList* commandList) {
	if (!isTarget_) {
		return;
	}

	// リソースモードに遷移
	isTarget_ = false;

	if (mode_ != RenderTextureMode::DsvOnly && resource_) {
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = resource_.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &barrier);
	}

	if (mode_ != RenderTextureMode::RtvOnly && depthResource_) {
		D3D12_RESOURCE_BARRIER depthbarrier{};
		depthbarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthbarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthbarrier.Transition.pResource = depthResource_.Get();
		depthbarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		depthbarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		depthbarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &depthbarrier);
	}
}

void RenderTexture::CreateColorTarget(uint32_t width, uint32_t height, DXGI_FORMAT format) {
	// カラーリソース作成
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = format;
	clearValue.Color[0] = clearValue.Color[1] = clearValue.Color[2] = 0.2f;
	clearValue.Color[3] = 1.0f;

	HRESULT hr = device_->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(&resource_)
	);
	assert(SUCCEEDED(hr) && "カラーリソースの作成に失敗しました");

	// rtvの完成
	rtvIndex_ = rtvManager_->CreateView(resource_.Get(), format);
	rtvHandle_ = rtvManager_->GetCPUHandle(rtvIndex_);

	// SRVの作成
	srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::System);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_CPU_DESCRIPTOR_HANDLE srvCPU = srvManager_->GetCPUHandle(srvIndex_);
	srvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex_));
	device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCPU);
}

void RenderTexture::CreateDepthTarget(uint32_t width, uint32_t height) {

	// 深度リソース作成
	depthResource_ = CreateDepthStencilTextureResource(device_, width, height);

	// dsvの作成
	dsvIndex_ = dsvManager_->CreateView(depthResource_.Get());
	dsvHandle_ = dsvManager_->GetCPUHandle(dsvIndex_);

	// 深度SRV作成
	dsvSrvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::System);

	D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
	depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;// 2Dテクスチャ
	depthSrvDesc.Texture2D.MipLevels = 1;
	
	D3D12_CPU_DESCRIPTOR_HANDLE depthSrvCPU = srvManager_->GetCPUHandle(dsvSrvIndex_);
	depthSrvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(dsvSrvIndex_));
	device_->CreateShaderResourceView(depthResource_.Get(), &depthSrvDesc, depthSrvCPU);
}

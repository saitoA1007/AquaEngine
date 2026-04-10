#include"RenderTexture.h"

using namespace GameEngine;

void RenderTexture::Initialize(RenderTextureContext& context) {
	rtvIndex_ = context.rtvIndex;
	srvIndex_ = context.srvIndex;
	width_ = context.width;
	height_ = context.height;
	resource_ = context.resource;
	rtvHandle_ = context.rtvHandle;
	srvGpuHandle_ = context.srvGpuHandle;

	// DSV
	depthResource_ = context.dsvResource;
	dsvIndex_ = context.dsvIndex;
	dsvHaveSrvIndex_ = context.dsvHaveSrvIndex;
	dsvHandle_ = context.dsvHandle;

	// 描画モード
	mode_ = context.mode;
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
		barrier.Transition.pResource = resource_;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);
	}

	// DSVの遷移
	if (mode_ != RenderTextureMode::RtvOnly && depthResource_) {
		D3D12_RESOURCE_BARRIER depthbarrier{};
		depthbarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthbarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthbarrier.Transition.pResource = depthResource_;
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
		barrier.Transition.pResource = resource_;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &barrier);
	}

	if (mode_ != RenderTextureMode::RtvOnly && depthResource_) {
		D3D12_RESOURCE_BARRIER depthbarrier{};
		depthbarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthbarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthbarrier.Transition.pResource = depthResource_;
		depthbarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		depthbarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		depthbarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &depthbarrier);
	}	
}

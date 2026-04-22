#include "RenderTexture.h"
#include <cassert>
#include "DepthStencilTexture.h"
using namespace GameEngine;

RtvManager* RenderTexture::rtvManager_ = nullptr;
DsvManager* RenderTexture::dsvManager_ = nullptr;

RenderTexture::~RenderTexture() {
	// カラーRTのデスクリプタ解放
	if (mode_ != RenderTextureMode::DsvOnly) {
		// RTVインデックス解放
		if (rtvIndex_ != 0) {
			rtvManager_->ReleseIndex(rtvIndex_);
		}
		// SRVインデックス解放
		if (srvIndex_ != 0) {
			srvManager_->ReleseIndex(srvIndex_);
		}
		// UAVインデックス解放
		if (uavIndex_ != 0) {
			srvManager_->ReleseIndex(uavIndex_);
		}
	}

	// 深度RTのデスクリプタ解放
	if (mode_ != RenderTextureMode::RtvOnly &&
		mode_ != RenderTextureMode::UavOnly &&
		mode_ != RenderTextureMode::RtvAndUav)
	{
		if (dsvIndex_ != 0) {
			dsvManager_->ReleseIndex(dsvIndex_);
		}

		// RtvAndDsvの場合、深度用SRVインデックスはsrvIndexとは別
		if (mode_ == RenderTextureMode::RtvAndDsv && dsvSrvIndex_ != 0) {
			srvManager_->ReleseIndex(dsvSrvIndex_);
		}
	}
}

void RenderTexture::Create(uint32_t width, uint32_t height, RenderTextureMode mode, DXGI_FORMAT colorFormat) {
	width_ = width;
	height_ = height;
	mode_ = mode;

	switch (mode_) {
	case RenderTextureMode::RtvOnly:
		CreateColorTarget(width, height, colorFormat);
		colorState_ = ColorResourceState::ShaderResource;
		break;

	case RenderTextureMode::DsvOnly:
		CreateDepthTarget(width, height);
		// DsvOnlyでは深度SRVをメインSRVとして扱う
		srvIndex_ = dsvSrvIndex_;
		srvGpuHandle_ = depthSrvGpuHandle_;
		break;

	case RenderTextureMode::RtvAndDsv:
		CreateColorTarget(width, height, colorFormat);
		CreateDepthTarget(width, height);
		colorState_ = ColorResourceState::ShaderResource;
		break;

	case RenderTextureMode::UavOnly:
		// UAV専用SRGB非対応フォーマットへ変換してリソース生成
		CreateUavTarget(width, height, ToUavCompatibleFormat(colorFormat));
		colorState_ = ColorResourceState::UnorderedAccess;
		break;

	case RenderTextureMode::RtvAndUav:
		// RTVとUAVを同一リソースにして両フラグを付与して生成する
		CreateColorTarget(width, height, ToUavCompatibleFormat(colorFormat));
		CreateUavTarget(width, height, ToUavCompatibleFormat(colorFormat));
		colorState_ = ColorResourceState::ShaderResource;
		break;

	default:
		assert(false && "未対応の RenderTextureMode です");
		break;
	}
}

void RenderTexture::TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList) {
	assert((mode_ == RenderTextureMode::RtvOnly || mode_ == RenderTextureMode::RtvAndDsv ||
		mode_ == RenderTextureMode::RtvAndUav || mode_ == RenderTextureMode::DsvOnly) &&
		"このモードはRTV遷移をサポートしていません");

	// 既にRTV状態
	if (colorState_ == ColorResourceState::RenderTarget) { return; }

	if (mode_ != RenderTextureMode::DsvOnly) {
		D3D12_RESOURCE_STATES stateBefore;
		if (colorState_ == ColorResourceState::UnorderedAccess) {
			stateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		} else {
			stateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		}

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = resource_.Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);
	}

	colorState_ = ColorResourceState::RenderTarget;

	// 深度リソースのRTV遷移
	if ((mode_ == RenderTextureMode::RtvAndDsv || mode_ == RenderTextureMode::DsvOnly) && depthResource_ && !isDepthTarget_) {
		D3D12_RESOURCE_BARRIER depthBarrier{};
		depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthBarrier.Transition.pResource = depthResource_.Get();
		depthBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		commandList->ResourceBarrier(1, &depthBarrier);
		isDepthTarget_ = true;
	}
}

void RenderTexture::TransitionToShaderResource(ID3D12GraphicsCommandList* commandList) {
	// 既にSRV状態
	if (colorState_ == ColorResourceState::ShaderResource && !isDepthTarget_) {return; }

	// カラー,UAVリソースのSRV遷移
	if (mode_ != RenderTextureMode::DsvOnly) {
		if (colorState_ != ColorResourceState::ShaderResource && resource_) {

			D3D12_RESOURCE_STATES stateBefore;
			if (colorState_ == ColorResourceState::UnorderedAccess) {
				stateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			} else {
				stateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			}

			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = resource_.Get();
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = stateBefore;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			commandList->ResourceBarrier(1, &barrier);
		}
	}

	colorState_ = ColorResourceState::ShaderResource;

	// 深度リソースのSRV遷移
	if ((mode_ == RenderTextureMode::DsvOnly || mode_ == RenderTextureMode::RtvAndDsv) && depthResource_ && isDepthTarget_) {
		D3D12_RESOURCE_BARRIER depthBarrier{};
		depthBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		depthBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		depthBarrier.Transition.pResource = depthResource_.Get();
		depthBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		depthBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		depthBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &depthBarrier);
		isDepthTarget_ = false;
	}
}

void RenderTexture::TransitionToUnorderedAccess(ID3D12GraphicsCommandList* commandList) {
	assert((mode_ == RenderTextureMode::UavOnly || mode_ == RenderTextureMode::RtvAndUav) &&
		"このモードは UAV 遷移をサポートしていません");

	// 既にUAV状態
	if (colorState_ == ColorResourceState::UnorderedAccess) { return; }

	D3D12_RESOURCE_STATES stateBefore;
	if (colorState_ == ColorResourceState::RenderTarget) {
		stateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	} else {
		stateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource_.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = stateBefore;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	commandList->ResourceBarrier(1, &barrier);

	colorState_ = ColorResourceState::UnorderedAccess;
}

void RenderTexture::InsertUavBarrier(ID3D12GraphicsCommandList* commandList) {
	assert((mode_ == RenderTextureMode::UavOnly || mode_ == RenderTextureMode::RtvAndUav) &&
		"このモードはUAVバリアをサポートしていません");

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = resource_.Get();
	commandList->ResourceBarrier(1, &barrier);
}

void RenderTexture::CreateColorTarget(uint32_t width, uint32_t height, DXGI_FORMAT format) {
	// カラーリソース作成
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	// RtvAndUavでは同一リソースにUAVフラグも立てる
	if (mode_ == RenderTextureMode::RtvAndUav) {
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Flags = flags;

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

void RenderTexture::CreateUavTarget(uint32_t width, uint32_t height, DXGI_FORMAT format) {

	// UavOnlyのみ新規リソースを作成する
	if (mode_ == RenderTextureMode::UavOnly) {

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
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		HRESULT hr = device_->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&resource_)
		);
		assert(SUCCEEDED(hr) && "UAVリソースの作成に失敗しました");

		// SRV作成
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

	// UAVビューを作成する
	assert(resource_ && "UAVビュー作成前にリソースが存在しません");

	uavIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::System);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	uavCpuHandle_ = srvManager_->GetCPUHandle(uavIndex_);
	uavGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(uavIndex_));
	device_->CreateUnorderedAccessView(resource_.Get(), nullptr, &uavDesc, uavCpuHandle_);

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

DXGI_FORMAT RenderTexture::ToUavCompatibleFormat(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:  return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:  return DXGI_FORMAT_B8G8R8A8_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:  return DXGI_FORMAT_B8G8R8X8_UNORM;
	default: return format;
	}
}

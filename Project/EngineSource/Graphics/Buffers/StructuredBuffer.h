#pragma once
#include "SrvResource.h"
#include "CreateBufferResource.h"
#include "Externals/DirectXTex/d3dx12.h"

namespace GameEngine {

	/// <summary>
	/// 構造化バッファ用
	/// </summary>
	template <typename T>
	class StructuredBuffer : public SrvResource {
	public:
		~StructuredBuffer() {
			// Unmap
			if (data_) {
				resource_->Unmap(0, nullptr);
				data_ = nullptr;
			}
			// srvの解放
			if (srvManager_) {
				srvManager_->ReleseIndex(srvIndex_);
			}
		}

		/// <summary>
		/// 要素数を指定してバッファを作成
		/// </summary>
		void Create(uint32_t numElements) {
			numElements_ = numElements;

			// リソースを作成
			resource_ = CreateBufferResource(device_, sizeof(T) * numElements_);
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));

			// SRVのインデックスを取得
			srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);
			// SRVの作成
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc.Buffer.NumElements = numElements_;
			srvDesc.Buffer.StructureByteStride = sizeof(T);
			srvHandleCPU_ = srvManager_->GetCPUHandle(srvIndex_);
			srvHandleGPU_ = srvManager_->GetGPUHandle(srvIndex_);
			device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvHandleCPU_);
		}

		T* GetData() const { return data_; }
		uint32_t GetNumElements() const { return numElements_; }
		const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetSrvHandleGPU() const { return srvHandleGPU_; }

	private:

		T* data_ = nullptr;
		uint32_t numElements_ = 0;

		uint32_t srvIndex_ = 0;
		// CPUのシェーダリソースビューのハンドル
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_;
		// GPUのシェーダリソースビューのハンドル
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_;
	};
}
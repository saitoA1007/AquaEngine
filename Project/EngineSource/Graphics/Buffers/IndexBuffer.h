#pragma once
#include <vector>
#include "SrvResource.h"
#include "CreateBufferResource.h"
#include "Externals/DirectXTex/d3dx12.h"
#include "ResourceGarbageCollector.h"

namespace GameEngine {

	/// <summary>
	/// インデックスデータ用クラス
	/// </summary>
	class IndexBuffer : public SrvResource {
	public:
		~IndexBuffer() {
			if (isCreated_) {
				// SRV インデックス解放
				if (srvManager_) {
					srvManager_->ReleseIndex(srvIndex_);
				}
				isCreated_ = false;
			}
		}

		void Create(const std::vector<uint32_t>& indices) {
			// インデックス数を取得
			totalIndices_ = static_cast<uint32_t>(indices.size());

			// インデックスバッファを作成
			resource_ = CreateBufferResource(device_, sizeof(uint32_t) * totalIndices_);
			// リソースの先頭のアドレスから使う
			indexBufferView_.BufferLocation = resource_->GetGPUVirtualAddress();
			// 使用するリソースのサイズはインデックス6つ分のサイズ
			indexBufferView_.SizeInBytes = sizeof(uint32_t) * totalIndices_;
			// インデックスはuint32_tとする
			indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

			// インデックスデータを生成
			uint32_t* indexData = nullptr;
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
			// インデックスデータをコピー
			std::memcpy(indexData, indices.data(), sizeof(uint32_t) * totalIndices_);

			// UnMapする
			resource_->Unmap(0, nullptr);
			indexData = nullptr;

			/// SRVの作成
			srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = totalIndices_;
			srvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			D3D12_CPU_DESCRIPTOR_HANDLE srvCPU = srvManager_->GetCPUHandle(srvIndex_);
			srvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex_));
			device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCPU);

			isCreated_ = true;
		}

		void Create(ID3D12GraphicsCommandList4* cmdList, const std::vector<uint32_t>& indices) {
			// インデックス数を取得
			totalIndices_ = static_cast<uint32_t>(indices.size());
			size_t sizeInBytes = sizeof(uint32_t) * totalIndices_;

			// DEFAULTヒープのリソースを作成
			resource_ = CreateBufferResource(
				device_,
				sizeInBytes,
				D3D12_HEAP_TYPE_DEFAULT,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_FLAG_NONE
			);

			// リソースの先頭のアドレスから使う
			indexBufferView_.BufferLocation = resource_->GetGPUVirtualAddress();
			// 使用するリソースのサイズはインデックス6つ分のサイズ
			indexBufferView_.SizeInBytes = sizeof(uint32_t) * totalIndices_;
			// インデックスはuint32_tとする
			indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

			// UPLOADヒープのステージングバッファを作成
			Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer = CreateBufferResource(
				device_,
				sizeInBytes,
				D3D12_HEAP_TYPE_UPLOAD,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				D3D12_RESOURCE_FLAG_NONE
			);

			// インデックスデータを生成
			uint32_t* indexData = nullptr;
			stagingBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
			// インデックスデータをコピー
			std::memcpy(indexData, indices.data(), sizeInBytes);
			// UnMapする
			stagingBuffer->Unmap(0, nullptr);
			indexData = nullptr;

			// GPU側でコピーコマンドを発行
			cmdList->CopyBufferRegion(resource_.Get(), 0, stagingBuffer.Get(), 0, sizeInBytes);

			// リソースの状態を遷移
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Transition.pResource = resource_.Get();
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			cmdList->ResourceBarrier(1, &barrier);

			// リソースの破棄を登録する
			ResourceGarbageCollector::GetInstance().Add(stagingBuffer);

			/// SRVの作成
			srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = totalIndices_;
			srvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			D3D12_CPU_DESCRIPTOR_HANDLE srvCPU = srvManager_->GetCPUHandle(srvIndex_);
			srvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex_));
			device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCPU);

			isCreated_ = true;
		}

		// ビューを取得
		const D3D12_INDEX_BUFFER_VIEW& GetView() const { return indexBufferView_; }

		// インデックス数
		uint32_t GetTotalIndices() const { return totalIndices_; }

		uint32_t GetSrvIndex() const { return srvIndex_; }
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return srvGpuHandle_; }
	private:
		// インデックスバッファビュー
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		// 頂点数
		uint32_t totalIndices_ = 0;

		uint32_t srvIndex_ = 0;
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_{};

		bool isCreated_ = false;
	};
}
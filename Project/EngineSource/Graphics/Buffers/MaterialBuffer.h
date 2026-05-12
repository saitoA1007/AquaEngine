#pragma once
#include "BufferRefResource.h"
#include "SrvResource.h"
#include "BufferRefManager.h"

namespace GameEngine {

	/// <summary>
	/// マテリアルを作成する構造体
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template <typename T>
	class MaterialBuffer : public BufferRefResource , public SrvResource {
	public:
		~MaterialBuffer() {
			// bufferRefの解放
			if (isCreated_) {
				bufferRefManager_->ReleseIndex(refIndex_);
			}
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
		/// マテリアルデータを作成
		/// </summary>
		/// <param name="type">マテリアルのタイプを設定</param>
		void Create(const uint32_t& type) {

			// 4バイト境界に切り上げたサイズを計算
			uint32_t bufferSize = (sizeof(T) + 3) & ~3;

			// マテリアルデータを作成
			resource_ = CreateBufferResource(device_, bufferSize);
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));

			// SRVのインデックスを取得
			srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);
			// SRVの作成
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.NumElements = bufferSize / 4;
			srvHandleCPU_ = srvManager_->GetCPUHandle(srvIndex_);
			srvHandleGPU_ = srvManager_->GetGPUHandle(srvIndex_);
			device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvHandleCPU_);

			// 参照用データを作成
			refIndex_ = bufferRefManager_->AllocateIndex();
			auto* data = bufferRefManager_->GetBufferRef(refIndex_);

			// bufferのスタート位置を取得
			uint32_t bufferStartIndex  = srvManager_->GetStartSrvIndex(SrvHeapType::Buffer);

			// マテリアルのsrv番号を設定
			data->type = type; // タイプを設定
			data->index = srvIndex_ - bufferStartIndex; // srvの番号を設定

			isCreated_ = true;
		}

		// アクセス用のsrvインデックス
		const uint32_t& GetRefIndex() const { return refIndex_; }
		T* GetData() const { return data_; }
		const uint32_t& GetSrvIndex() const { return srvIndex_; }
		const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetSrvHandleGPU() const { return srvHandleGPU_; }

	private:
		uint32_t refIndex_ = 0;

		T* data_ = nullptr;

		uint32_t srvIndex_ = 0;
		// CPUのシェーダリソースビューのハンドル
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_;
		// GPUのシェーダリソースビューのハンドル
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_;

		bool isCreated_ = false;
	};
}
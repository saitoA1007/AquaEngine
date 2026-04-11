#pragma once
#include "GpuResource.h"
#include "CreateBufferResource.h"

namespace GameEngine {

	/// <summary>
	/// 定数バッファ用クラス
	/// </summary>
	template <typename T>
	class ConstantBuffer : public GpuResource {
	public:
		~ConstantBuffer() {
			// デストラクタで自動的にUnmapを実行
			if (data_) {
				resource_->Unmap(0, nullptr);
				data_ = nullptr;
			}
		}

		void Create(ID3D12Device* device) {
			resource_ = CreateBufferResource(device, sizeof(T));

			// データを書き込むためのポインタを取得し、保持し続ける
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
		}

		// データを取得する
		T* GetData() const { return data_; }

	private:
		// データのポインタ
		T* data_ = nullptr;
	};
}
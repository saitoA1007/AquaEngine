#pragma once
#include <memory>
#include "GpuResource.h"
#include "DynamicBuffer.h"

namespace GameEngine {

	/// <summary>
	/// 定数バッファ用クラス
	/// </summary>
	template <typename T>
	class ConstantBuffer : public GpuResource {
	public:
		ConstantBuffer() = default;

		~ConstantBuffer() override {
			Release();
		}

		void Create() {
			Create(1);
		}

		void Create(uint32_t num) {
			Release();

			buffer_ = CreateDynamicBuffer(sizeof(T) * num);
			resource_ = buffer_->GetResourcePtr();

			// データを書き込むためのポインタを取得し、保持し続ける
			data_ = buffer_->GetData<T>().data();
		}

		// 解放
		void Release() {
			data_ = nullptr;
			resource_.Reset();
			buffer_.reset();
		}

		// データを取得する
		T* GetData() const { return data_; }

	private:
		// CPU書き込みとGPU転送を管理
		std::shared_ptr<DynamicBuffer> buffer_;
		// データのポインタ
		T* data_ = nullptr;
	};
}

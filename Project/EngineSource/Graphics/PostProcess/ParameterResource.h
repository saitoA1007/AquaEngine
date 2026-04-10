#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cassert>
#include"CreateBufferResource.h"

namespace GameEngine {

	template<typename T>
	class ParameterResource final {
	public:

		/// <summary>
		/// リソースを作成
		/// </summary>
		/// <param name="device"></param>
		void CreateResource(ID3D12Device* device) {

			// リソースを作成
			bufferResource_ = CreateBufferResource(device, sizeof(T));
			// 書き込むためのアドレスを取得
			bufferResource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
			if (data_) {
				*data_ = T();
			}
		}

		/// <summary>
		/// リソースを取得
		/// </summary>
		/// <returns></returns>
		ID3D12Resource* GetResource() const { return bufferResource_.Get(); }

		/// <summary>
		/// データを取得
		/// </summary>
		/// <returns></returns>
		T* GetData() const { return data_; }

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource_;
		T* data_ = nullptr;
	};
}
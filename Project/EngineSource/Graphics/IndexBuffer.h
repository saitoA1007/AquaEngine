#pragma once
#include <vector>
#include "GpuResource.h"
#include "CreateBufferResource.h"

namespace GameEngine {

	/// <summary>
	/// インデックスデータ用クラス
	/// </summary>
	class IndexBuffer : public GpuResource {
	public:

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
		}

		// ビューを取得
		const D3D12_INDEX_BUFFER_VIEW& GetView() const { return indexBufferView_; }

		// インデックス数
		uint32_t GetTotalIndices() const { return totalIndices_; }
	private:
		// インデックスバッファビュー
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		// 頂点数
		uint32_t totalIndices_ = 0;
	};
}
#pragma once
#include <vector>
#include "GpuResource.h"
#include "CreateBufferResource.h"

namespace GameEngine {

	/// <summary>
	/// 頂点データ用クラス
	/// </summary>
	template <typename T>
	class VertexBuffer : public GpuResource {
	public:
		~VertexBuffer() {
			// マッピングを解除する
			if (vertexData_) {
				resource_->Unmap(0, nullptr);
				vertexData_ = nullptr;
			}
		}

		void Create(ID3D12Device* device, const std::vector<T>& vertices) {
			// 頂点数を取得
			totalVertices_ = static_cast<uint32_t>(vertices.size());

			// 頂点バッファを作成
			resource_ = CreateBufferResource(device, sizeof(T) * totalVertices_);
			// リソースの先頭のアドレスから使う
			vertexBufferView_.BufferLocation = resource_->GetGPUVirtualAddress();
			// 使用するリソースのサイズは頂点3つ分のサイズ
			vertexBufferView_.SizeInBytes = sizeof(T) * totalVertices_;
			// 1頂点あたりのサイズ
			vertexBufferView_.StrideInBytes = sizeof(T);

			// 頂点データを生成
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
			// 頂点データをコピー
			std::memcpy(vertexData_, vertices.data(), sizeof(T) * totalVertices_);
		}

		// ビューを取得
		const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return vertexBufferView_; }

		// 頂点数
		uint32_t GetTotalVertices() const { return totalVertices_; }
	private:
		// 頂点バッファビュー
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		// メッシュデータ
		T* vertexData_ = nullptr;
		// 頂点数
		uint32_t totalVertices_ = 0;
	};
}
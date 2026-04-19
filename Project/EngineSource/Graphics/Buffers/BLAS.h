#pragma once
#include "GpuResource.h"
#include <cstdint>

namespace GameEngine {

	class BLAS : public GpuResource {
	public:
		BLAS() = default;
		~BLAS() = default;

		void Create(ID3D12GraphicsCommandList4* cmdList,
			const D3D12_VERTEX_BUFFER_VIEW& vertexBufView, const D3D12_INDEX_BUFFER_VIEW& indexBufView,
			const uint32_t& totalVertices, const uint32_t& totalIndices);

	private:
		// BLAS構築に必要な一時的な作業用メモリ。GPUの終了時に削除してよい
		Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer_;
	};
}
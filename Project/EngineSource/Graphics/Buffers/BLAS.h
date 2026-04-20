#pragma once
#include "GpuResource.h"
#include <cstdint>

namespace GameEngine {

	class BLAS : public GpuResource {
	public:
		BLAS() = default;
		~BLAS() = default;

		/// <summary>
		/// BLASを作成する
		/// </summary>
		void Create(ID3D12GraphicsCommandList4* cmdList,
			const D3D12_VERTEX_BUFFER_VIEW& vertexBufView, const D3D12_INDEX_BUFFER_VIEW& indexBufView,
			const uint32_t& totalVertices, const uint32_t& totalIndices);
	};
}
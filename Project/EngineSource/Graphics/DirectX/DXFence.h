#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

namespace GameEngine {

	class DXFence final {
	public:

		DXFence() = default;
		~DXFence() = default;

		void Initialize(ID3D12Device* device);

		void WaitForGPU(ID3D12CommandQueue* commandQueue);

	private:
		Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
		uint64_t fenceValue_ = 0;
		HANDLE fenceEvent_ = nullptr;
	};
}


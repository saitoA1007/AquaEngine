#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

namespace GameEngine {

	class DXFence final {
	public:

		DXFence() = default;
		~DXFence();

		void Initialize(ID3D12Device* device);

		// キューにSignalを積み、そのフェンス値を返す
		uint64_t Signal(ID3D12CommandQueue* commandQueue);

		// 指定したフェンス値にGPUが到達するまでCPUを待たせる
		void WaitForValue(uint64_t value);

		// CPUを完全に止めてGPUの処理完了を待ち同期させる
		void WaitForGPU(ID3D12CommandQueue* commandQueue);

		// GPUが既に実行完了した最新のフェンス値を取得
		uint64_t GetCompletedValue() const { return fence_->GetCompletedValue(); }

		// 次にSignalされるフェンス値を取得。現在記録中のフレームの終わりにSignalされる値
		uint64_t GetNextFenceValue() const { return fenceValue_ + 1; }

	private:
		Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
		uint64_t fenceValue_ = 0;
		HANDLE fenceEvent_ = nullptr;
	};
}

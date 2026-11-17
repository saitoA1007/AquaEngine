#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "FrameContext.h"

namespace GameEngine {

	class DXCommand final {
	public:

		DXCommand() = default;
		~DXCommand() = default;

		void Initialize(ID3D12Device* device);

		void Close();

		// メインのコマンドリストを実行する。preCommandListがあれば先に別のExecuteで実行する
		void Execute(ID3D12CommandList* preCommandList = nullptr);

		// 指定したフレームスロットのアロケータでコマンドリストを準備する
		void Reset(uint32_t frameIndex);

		ID3D12CommandQueue* GetQueue() const { return commandQueue_.Get(); }

		ID3D12GraphicsCommandList4* GetCommandList() const { return commandList_.Get(); }

	private:

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
		// フレームスロットごとのコマンドアロケータ
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[kFrameCount];
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> commandList_;
	};
}

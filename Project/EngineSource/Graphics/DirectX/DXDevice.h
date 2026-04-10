#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

namespace GameEngine {

	class DXDevice final {
	public:

		DXDevice() = default;
		~DXDevice() = default;

		void Initialize();

		ID3D12Device* GetDevice() const { return device_.Get(); }
		IDXGIFactory7* GetFactory() const { return dxgiFactory_.Get(); }

	private:

		Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
		Microsoft::WRL::ComPtr<ID3D12Device> device_;

	private:

		/// <summary>
		/// ファクトリーを作成
		/// </summary>
		void CreateFactory();

		/// <summary>
		/// デバイスを作成
		/// </summary>
		void CreateDevice();

#ifdef _DEBUG

		/// <summary>
		/// デバックレイヤーを作成
		/// </summary>
		void CreateDebugLayer();
#endif
	};
}


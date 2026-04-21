#pragma once
#include <array>
#include "Externals/DirectXTex/d3dx12.h"
#include "DXC.h"
#include "RenderPassController.h"

namespace GameEngine {


	/// <summary>
	/// テスト用に作成した。レイトレーシングの描画の大まかな流れ
	/// </summary>
	class TestRayPipeline {
	public:

		void Initialize();

		void Draw();

	private:
		ID3D12Device5* device_ = nullptr;
		ID3D12GraphicsCommandList4* commandList_ = nullptr;
		DXC* dxc_ = nullptr;
		RenderPassController* renderPassController_ = nullptr;

		// グローバルのルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureGlobal_;

		// ステートオブジェクト
		Microsoft::WRL::ComPtr<ID3D12StateObject> stateObject_;

		// シェーダーテーブル
		Microsoft::WRL::ComPtr<ID3D12Resource> shaderTable_;

		// レイトレーシングを開始する時に衣装する構造体
		D3D12_DISPATCH_RAYS_DESC dispatchRayDesc_;

		std::wstring DefaultHitgroup_ = L"DefaultHitGroup";

	private:

		// ルートシグネチャ(Global) を生成します.
		// ルートシグネチャの仕組みはラスタライズ用PSOと変わらない。
		// 既存の仕組みを活かせるかも
		void CreateRootSignatureGlobal();

		// レイトレーシング用の StateObject を構築します.
		void CreateStateObject();

		// レイトレーシング結果書き込み用バッファを生成します.
		void CreateResultBuffer();

		// レイトレーシングで使用する ShaderTable を構築します.
		// レイトレーシングパイプラインで使用する全てのシェーダーの情報が格納されたデータ
		// クラスかする時は単一で持つように設計をおこなう
		void CreateShaderTable();

	private:

		UINT RoundUp(size_t size, UINT align) {
			return UINT(size + align - 1) & ~(align - 1);
		}

		void CreateBuffer(int tableSize) {
			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Alignment = 0;
			resourceDesc.Width = tableSize;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			HRESULT hr = device_->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(shaderTable_.ReleaseAndGetAddressOf())
			);
			if (FAILED(hr)) {
				throw std::runtime_error("Failed to create shader table buffer.");
			}
		}
	};
}

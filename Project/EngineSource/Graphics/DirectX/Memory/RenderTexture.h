#pragma once
#include"RtvManager.h"
#include"SrvManager.h"

namespace GameEngine {

	// 描画タイプ
	enum class RenderTextureMode {
		RtvOnly,    // RTVのみ
		DsvOnly,    // Dsvのみ
		RtvAndDsv   // RTVとDSV
	};

	// レンダーターゲットを作成する土台
	struct RenderTextureContext {
		uint32_t rtvIndex = 0;
		uint32_t srvIndex = 0;
		uint32_t dsvIndex = 0;
		uint32_t width = 0;
		uint32_t height = 0;

		RenderTextureMode mode = RenderTextureMode::RtvAndDsv;
		uint32_t dsvHaveSrvIndex = 0;

		ID3D12Resource* resource = nullptr;
		ID3D12Resource* dsvResource = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = {};
	};

	class RenderTexture {
	public:
		RenderTexture() = default;
		~RenderTexture() = default;

		void Initialize(RenderTextureContext& context);

		// レンダーターゲットに変更
		void TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList);
		
		// シェーダーリソースに変更
		void TransitionToShaderResource(ID3D12GraphicsCommandList* commandList);

		D3D12_CPU_DESCRIPTOR_HANDLE& GetRtvHandle() { return rtvHandle_; }

		CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() { return srvGpuHandle_; }

		D3D12_CPU_DESCRIPTOR_HANDLE& GetDsvHandle() { return dsvHandle_; }

		uint32_t GetSrvIndex() { return srvIndex_; }

		uint32_t GetRtvIndex() const { return rtvIndex_; }

		ID3D12Resource* GetResource() const { return resource_; }

		uint32_t GetWidth() const { return width_; }

		uint32_t GetHeight() const { return height_; }

		RenderTextureMode GetMode() const { return mode_; }

	private:

		// インデックス
		uint32_t rtvIndex_ = 0;
		uint32_t srvIndex_ = 0;
		uint32_t dsvIndex_ = 0;
		uint32_t dsvHaveSrvIndex_ = 0;

		uint32_t width_ = 0;
		uint32_t height_ = 0;

		// リソース
		ID3D12Resource* resource_ = nullptr;

		// 深度リソース
		ID3D12Resource* depthResource_ = nullptr;

		// rtvハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_;

		// dsvハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_;

		// srvハンドル
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_;

		// 現在の描画状態
		bool isTarget_ = false;

		// 通常描画
		RenderTextureMode mode_ = RenderTextureMode::RtvAndDsv;
	};

}
#pragma once
#include "SrvResource.h"
#include "RtvManager.h"
#include "DsvManager.h"

namespace GameEngine {

	// 描画タイプ
	enum class RenderTextureMode {
		RtvOnly,    // RTVのみ
		DsvOnly,    // Dsvのみ
		RtvAndDsv   // RTVとDSV
	};

	class RenderTexture : public SrvResource {
	public:
		RenderTexture() = default;
		~RenderTexture();

		static void StaticInitialize(RtvManager* rtvManager, DsvManager* dsvManager) {
			rtvManager_ = rtvManager;
			dsvManager_ = dsvManager;
		}

		void Create(
			uint32_t width, uint32_t height,
			RenderTextureMode mode = RenderTextureMode::RtvAndDsv,
			DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
		);

		// レンダーターゲットに変更
		void TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList);
		// シェーダーリソースに変更
		void TransitionToShaderResource(ID3D12GraphicsCommandList* commandList);

		D3D12_CPU_DESCRIPTOR_HANDLE& GetRtvHandle() { return rtvHandle_; }
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() { return srvGpuHandle_; }
		D3D12_CPU_DESCRIPTOR_HANDLE& GetDsvHandle() { return dsvHandle_; }

		uint32_t GetRtvIndex() const { return rtvIndex_; }
		uint32_t GetSrvIndex() { return srvIndex_; }

		uint32_t GetWidth() const { return width_; }
		uint32_t GetHeight() const { return height_; }

		RenderTextureMode GetMode() const { return mode_; }

	private:
		RenderTexture(const RenderTexture&) = delete;
		RenderTexture& operator=(const RenderTexture&) = delete;
		RenderTexture(RenderTexture&&) = delete;
		RenderTexture& operator=(RenderTexture&&) = delete;

		static RtvManager* rtvManager_;
		static DsvManager* dsvManager_;

		// インデックス
		uint32_t rtvIndex_ = 0;
		uint32_t srvIndex_ = 0;
		// rtvハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_;
		// srvハンドル
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_;

		// 深度リソース
		Microsoft::WRL::ComPtr<ID3D12Resource> depthResource_ = nullptr;
		uint32_t dsvIndex_ = 0;
		uint32_t dsvSrvIndex_ = 0;
		// dsvハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_;
		CD3DX12_GPU_DESCRIPTOR_HANDLE   depthSrvGpuHandle_ = {};

		uint32_t width_ = 0;
		uint32_t height_ = 0;
		// 通常描画
		RenderTextureMode mode_ = RenderTextureMode::RtvAndDsv;
		// 現在の描画状態
		bool isTarget_ = false;
	private:

		/// <summary>
		/// カラーリソースとそのRTV,SRVを作成する
		/// </summary>
		/// <param name="width"></param>
		/// <param name="height"></param>
		/// <param name="format"></param>
		void CreateColorTarget(uint32_t width, uint32_t height, DXGI_FORMAT format);

		/// <summary>
		/// 深度リソースとそのDSV,SRVを作成する
		/// </summary>
		/// <param name="width"></param>
		/// <param name="height"></param>
		void CreateDepthTarget(uint32_t width, uint32_t height);
	};
}
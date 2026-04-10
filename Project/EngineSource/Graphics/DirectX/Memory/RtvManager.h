#pragma once
#include <d3d12.h>
#include <wrl.h>
#include<vector>
#include <deque>
#include"Externals/DirectXTex/d3dx12.h"

namespace GameEngine {

	// 設定項目
	struct RtvContext {
		uint32_t width = 1280;
		uint32_t height = 720;
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		uint32_t mipLevels = 1;
		D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		bool allowUAV = false; // Compute Shaderでの使用を許可
	};

	class RtvManager {
	public:
		RtvManager() = default;
		~RtvManager() = default;

		void Initialize(ID3D12Device* device);

		/// <summary>
		/// 生成
		/// </summary>
		/// <param name="context"></param>
		/// <returns></returns>
		uint32_t CreateRenderTargetResource(RtvContext context = RtvContext(1280,720, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,false));

		/// <summary>
		/// インデックスを削除
		/// </summary>
		/// <param name="index"></param>
		void ReleseIndex(const uint32_t& index);

		ID3D12Resource* GetResource(const uint32_t& index);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(const uint32_t& index) const;

		uint32_t GetDescriptorSize() const { return descriptorSizeRTV_; }
			
	private:
		RtvManager(const RtvManager&) = delete;
		RtvManager& operator=(const RtvManager&) = delete;

		ID3D12Device* device_ = nullptr;

		// rtvヒープ
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;

		float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
		 D3D12_CLEAR_VALUE clearValue_{};

		// 最大RTV数
		uint32_t maxRtvCount_ = 128;

		uint32_t descriptorSizeRTV_ = 0;

		// 現在使用中のRTVインデックス
		uint32_t currentRtvIndex_ = 0;

		// 解放されたインデックスのリスト
		std::deque<uint32_t> freeIndices_;

		// リソース
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> resources_;
	};

}
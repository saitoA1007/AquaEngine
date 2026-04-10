#pragma once
#include <unordered_map>
#include"RtvManager.h"
#include"SrvManager.h"
#include"DsvManager.h"
#include"RenderTexture.h"

namespace GameEngine {

	class RenderTextureManager {
	public:
		RenderTextureManager() = default;
		~RenderTextureManager() = default;

		void Initialize(RtvManager* rtvMasnager, SrvManager* srvManager,DsvManager* dsvmanager, ID3D12Device* device);

		void Create(const std::string& name, RenderTextureMode mode,RtvContext context = RtvContext(1280, 720, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, false));

		RenderTexture* GetRenderTexture(const std::string& name);

		void Release(const std::string& name);

	private:
		ID3D12Device* device_ = nullptr;
		RtvManager* rtvManager_ = nullptr;
		SrvManager* srvManager_ = nullptr;
		DsvManager* dsvmanager_ = nullptr;
		std::unordered_map<std::string, std::unique_ptr<RenderTexture>> renderTextures_;
	};
}
#pragma once
#include <memory>
#include <vector>
#include<unordered_map>
#include <string>
#include"RenderPass.h"

#include"RenderTextureManager.h"

namespace GameEngine {

	class RenderPassController final {
	public:
		RenderPassController() = default;
		~RenderPassController() = default;

		void Initialize(RenderTextureManager* renderTextureManager, ID3D12GraphicsCommandList* commandList);

		// パスを作成する
		void AddPass(const std::string& name, RenderTextureMode mode = RenderTextureMode::RtvAndDsv,uint32_t wid = 1280,uint32_t hei = 720);

		// 描画前に呼び出す(参照する時に切り替えられていなければassertで引っ掛ける)
		void PrePass(const std::string& name);
		void PostPass(const std::string& name);

		// 最後に描画するパスを設定する
		void SetEndPass(const std::string& name);

		// 最終敵な描画先
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetFinalOutputSRV() { return resultSrvHandle_; }

		CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvHandle(const std::string& name);

		uint32_t GetSrvIndex(const std::string& name);

	private:
		RenderPassController(const RenderPassController&) = delete;
		RenderPassController& operator=(const RenderPassController&) = delete;

		RenderTextureManager* renderTextureManager_ = nullptr;
		
		std::unordered_map<std::string, std::unique_ptr<RenderPass>> renderPassList_;

		ID3D12GraphicsCommandList* commandList_ = nullptr;

		std::string resultPassName_ = "";
		CD3DX12_GPU_DESCRIPTOR_HANDLE resultSrvHandle_;

		uint32_t width_ = 0;
		uint32_t height_ = 0;
	};
}


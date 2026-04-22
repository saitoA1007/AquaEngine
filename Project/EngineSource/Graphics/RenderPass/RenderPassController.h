#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "RenderPass.h"
#include "RenderTextureManager.h"

namespace GameEngine {

	class RenderPassController final {
	public:
		RenderPassController() = default;
		~RenderPassController() = default;

		// 初期化処理
		void Initialize(RenderTextureManager* renderTextureManager, ID3D12GraphicsCommandList* commandList);

		// パスを作成する
		void AddPass(const std::string& name, RenderTextureMode mode = RenderTextureMode::RtvAndDsv,uint32_t wid = 1280,uint32_t hei = 720);

		// 描画前に呼び出す(参照する時に切り替えられていなければassertで引っ掛ける)
		void PrePass(const std::string& name);
		void PostPass(const std::string& name);
		void SwitchToUnorderedAccess(const std::string& name);
		void InsertUavBarrier(const std::string& name);

		// 描画の最終パスの設定
		void SetSceneFinalPass(const std::string& name);
		const std::string& GetSceneFinalPass() const { return sceneFinalPassName_; }

		// ポストエフェクトの最終パスの設定
		void SetPostProcessFinalPass(const std::string& name);
		const std::string& GetPostProcessFinalPass() const { return postProcessFinalPassName_; }

		// 最終的に画面に出すためのパスの設定
		void SetPresentPass(const std::string& name);
		const std::string& GetPresentPass() const { return presentPassName_; }

		CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvHandle(const std::string& name);
		uint32_t GetSrvIndex(const std::string& name);
	private:
		RenderPassController(const RenderPassController&) = delete;
		RenderPassController& operator=(const RenderPassController&) = delete;

		RenderTextureManager* renderTextureManager_ = nullptr;
		ID3D12GraphicsCommandList* commandList_ = nullptr;

		std::unordered_map<std::string, std::unique_ptr<RenderPass>> renderPassList_;

		// 描画の最終パス
		std::string sceneFinalPassName_ = "";
		CD3DX12_GPU_DESCRIPTOR_HANDLE sceneFinalPassSrvHandle_;
		// ポストエフェクトの最終パス
		std::string postProcessFinalPassName_ = "";
		CD3DX12_GPU_DESCRIPTOR_HANDLE postProcessFinalPassSrvHandle_;
		// 最終的に画面に出すためのパス
		std::string presentPassName_ = "";
		CD3DX12_GPU_DESCRIPTOR_HANDLE presentPassSrvHandle_;

		uint32_t width_ = 0;
		uint32_t height_ = 0;
	};
}


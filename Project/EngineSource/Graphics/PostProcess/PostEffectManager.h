#pragma once
#include "SrvManager.h"
#include "PSO/Core/PSOManager.h"
#include "ConstantBuffer.h"
#include "RenderPass/RenderPassController.h"
#include "IPostEffect.h"

namespace GameEngine {

	class PostEffectManager {
	public:
		// ポストエフェクトタイプ
		enum class PostEffectType {
			kBloom,
			kVignetting,
			kRadialBlur,

			kMaxCount,
		};

	public:

		// 初期化処理
		void Initialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager, PSOManager* psoManager, RenderPassController* renderPassController);

		// 描画コマンドの解放
		void Execute();

		// エフェクトを追加
		template <class T>
		T* AddPostEffect(const std::string& passName, const std::string& psoName) {
			auto effect = std::make_unique<T>();
			uint32_t srvIndex = renderPassController_->GetSrvIndex(passName);
			// 登録する
			effect->Register(srvIndex, psoName);

			T* ptr = effect.get();
			drawQueueList_[passName] = effect.get();
			effects_[passName] = std::move(effect);
			return ptr;
		}

	private:
		ID3D12GraphicsCommandList* commandList_ = nullptr;
		SrvManager* srvManager_ = nullptr;
		RenderPassController* renderPassController_ = nullptr;

		std::string currentPassName_ = "";
		uint32_t currentPassIndex_ = 0;

		// エフェクトデータ
		std::unordered_map<std::string, std::unique_ptr<IPostEffect>> effects_;

		// 描画リスト [描画パス][ポストエフェクトデータ]
		std::unordered_map<std::string, IPostEffect*> drawQueueList_;

		// 描画パスの実行順
		std::vector<std::string> passExecuteOrder_;

		// psoのリスト
		std::unordered_map<std::string, DrawPsoData> psoList_;
	private:

		// psoを登録する
		void RegisterPSO(const std::string& name, PSOManager* psoManager) {
			psoList_[name] = psoManager->GetDrawPsoData(name);
		}

		// パスの実行順を登録
		void RegisterPassOrder(const std::vector<std::string>& order) {
			passExecuteOrder_ = order;
		}

		// 文字列キーでPSOをセット
		void PreDraw(const std::string& psoName);
	};
}
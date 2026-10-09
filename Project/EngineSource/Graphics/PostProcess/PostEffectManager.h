#pragma once
#include <functional>
#include "ConstantBuffer.h"
#include "SrvManager.h"
#include "PSO/Core/PSOManager.h"
#include "RenderPass/RenderPassController.h"
#include "IPostEffect.h"
#include "PostEffectData.h"
#include "NodeSystem/PostEffectGraph.h"
#include "JsonSerializer.h"

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

		struct Resource {
			uint32_t srvIndex = 0;
			int poolSlot = -1;
			int refCount = 0;
			std::string passName; // 実際に描かれたパス名
		};

	public:

		// 初期化処理
		void Initialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager, PSOManager* psoManager, RenderPassController* renderPassController);

		// 描画コマンドの解放
		void Execute();

		// エフェクトの種類を登録する
		template <class T>
		void RegisterEffectType(const std::string& typeName, PSOManager* psoManager) {
			RegisterPSO(typeName, psoManager);
			factories_[typeName] = [typeName]() {
				auto e = std::make_unique<T>();
				e->Register(0, typeName);
				return e;
				};
		}

		template <class T>
		T* GetPostEffect(const std::string& name) {
			auto it = effects_.find(name);
			if (it != effects_.end()) {
				return dynamic_cast<T*>(it->second.get());
			}
			return nullptr;
		}

		// 追加メニュー用
		std::vector<std::string> GetEffectTypeNames() const {
			std::vector<std::string> names;
			for (auto& [k, v] : factories_) { names.push_back(k); }
			std::sort(names.begin(), names.end());
			return names;
		}

		const std::unordered_map<std::string, std::unique_ptr<IPostEffect>>& GetEffects() const { return effects_; }

		// ノードデータを受け取る
		PostEffectGraph& GetGraph() { return graph_; }

		// ノードデータをリセット
		void ResetGraph() {
			BuildDefaultGraph();
			executeOrder_.clear();
		}

		// ノードを作る
		PostEffectNode* CreateEffectNode(const std::string& typeName, const std::string& name = "");
		// ノードを消す
		void DestroyEffectNode(int nodeId);
		// 保存パスの切り替え
		void SetPersistent(int nodeId, bool enable);

		// ノードの保存
		bool SaveGraph(const std::string& filePath = kGraphFilePath);
		// ノードのロード
		bool LoadGraph(const std::string& filePath = kGraphFilePath);

		// パス
		const std::string& GetFinalPassName() const { return currentPassName_; }

	private:

		// 保存先
		static constexpr const char* kGraphFilePath = "EngineSource/Resources/Json/PostEffectGraph.json";

		// 使い回し用のパス
		static constexpr const uint32_t kPoolSize = 3;
		const std::array<std::string, kPoolSize> poolPassNames_ = {
			"PostProcess_RT0", "PostProcess_RT1", "PostProcess_RT2"
		};

	private:
		ID3D12GraphicsCommandList* commandList_ = nullptr;
		SrvManager* srvManager_ = nullptr;
		RenderPassController* renderPassController_ = nullptr;

		std::string currentPassName_ = "";
		//uint32_t currentPassIndex_ = 0;

		// エフェクトデータ
		std::unordered_map<std::string, std::unique_ptr<IPostEffect>> effects_;

		// 各エフェクトの生成
		std::unordered_map<std::string, std::function<std::unique_ptr<IPostEffect>()>> factories_;

		// psoのリスト
		std::unordered_map<std::string, DrawPsoData> psoList_;

		// 作られている描画パスを管理
		std::unordered_set<std::string> createdPasses_;

		// ノードデータ
		PostEffectGraph graph_;
		// ノードIDの実行順
		std::vector<int> executeOrder_;

		// 削除したエフェクトの遅延破棄
		static constexpr int kRetireFrames = 3;   // フレームバッファ数以上にする
		struct Retired { std::unique_ptr<IPostEffect> fx; int framesLeft; };
		std::vector<Retired> retired_;

	private:

		// psoを登録する
		void RegisterPSO(const std::string& name, PSOManager* psoManager) {
			psoList_[name] = psoManager->GetDrawPsoData(name);
		}

		// 作成していなければ描画パスを作る
		void EnsurePass(const std::string& name) {
			if (createdPasses_.insert(name).second) {
				renderPassController_->AddPass(name);
			}
		}

		void BuildDefaultGraph();
		void RebuildOrder();

		// 破棄
		void RetireAllEffects();

		// 文字列キーでPSOをセット
		void PreDraw(const std::string& psoName);
	};
}

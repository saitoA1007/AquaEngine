#pragma once
#include <functional>
#include "IEditorWindow.h"
#include "NodeSystem/MaterialGraph.h"
#include "ImGuiManager.h"
namespace ned = ax::NodeEditor;

namespace GameEngine {

	// 前方宣言
	class PSOManager;

	class MaterialNodeWindow : public IEditorWindow {
	public:
		MaterialNodeWindow(PSOManager* psoManager);
		~MaterialNodeWindow() = default;

		void Draw() override;
		std::string GetName() const override { return "MaterialNodeWindow"; }

		void Render(MaterialGraph& graph);

	private:
		PSOManager* psoManager_ = nullptr;
		ned::EditorContext* context_ = nullptr;
		MaterialGraph* graph_ = nullptr;

		// テスト用の一時的なグラフ
		MaterialGraph testgraph_;

		// 接続開始ピン
		int newLinkPin_ = -1;

		bool dirtyFlag_ = false;

		// 登録されているノード
		std::unordered_map<std::string, std::function<void(MaterialGraph&)>> registerNode_;

	private:

		void HandleLinkCreation(MaterialGraph& graph);

		void HandleLinkDeletion(MaterialGraph& graph);

		void HandleContextMenu(MaterialGraph& graph);

		// ノードを登録
		template<typename T>
		void RegisterNode(std::string nodeName) {
			registerNode_[nodeName] = [](MaterialGraph& graph) {
				graph.nodes.push_back(std::make_unique<T>(graph));
			};
		}
	};

	// ヘルパー関数
	namespace {

		// 接続の判定をする
		bool CanConnect(const MaterialGraph& graph, int startPinId, int endPinId);
	}
}


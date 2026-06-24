#pragma once
#include "IEditorWindow.h"
#include "NodeSystem/MaterialGraph.h"
#include "ImGuiManager.h"
namespace ned = ax::NodeEditor;

namespace GameEngine {

    class MaterialNodeWindow : public IEditorWindow {
    public:
        MaterialNodeWindow();
        ~MaterialNodeWindow() = default;

        void Draw() override;
        std::string GetName() const override { return "MaterialNodeWindow"; }

        void Render(MaterialGraph& graph);

    private:
        ned::EditorContext* context_ = nullptr;
        MaterialGraph* graph_ = nullptr;

        // テスト用の一時的なグラフ
        MaterialGraph testgraph_;

        // 接続開始ピン
        int newLinkPin_ = -1;

        bool dirtyFlag_ = false;

    private:

        void HandleLinkCreation(MaterialGraph& graph);

        void HandleLinkDeletion(MaterialGraph& graph);

        void HandleContextMenu(MaterialGraph& graph);
    };

    // ヘルパー関数
    namespace {

        // グラフ内のピンIDから対称のPinを探す
        const Pin* FindPin(const MaterialGraph& graph, int pinId);

        // 接続の判定をする
        bool CanConnect(const MaterialGraph& graph, int startPinId, int endPinId);
    }
}


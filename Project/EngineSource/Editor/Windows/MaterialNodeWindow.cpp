#include "MaterialNodeWindow.h"
#include "NodeSystem/MaterialNode.h"
using namespace GameEngine;

MaterialNodeWindow::MaterialNodeWindow(PSOManager* psoManager) {
    // pso管理を取得
    psoManager_ = psoManager;

    // マテリアルノードレイアウト設定
    ned::Config cfg;
    cfg.SettingsFile = "MaterialEditor.json";
    context_ = ned::CreateEditor(&cfg);

    // 各ノードを登録
    RegisterNode<MathNode>("Math");
    RegisterNode<ConstantNode>("Constant");
    RegisterNode<TextureSampleNode>("TextureSample");
}

void MaterialNodeWindow::Draw() {
    if (!isActive) return;

    ImGui::Begin("MaterialNodeWindow", &isActive);
    Render(testgraph_);
    ImGui::End();
}

void MaterialNodeWindow::Render(MaterialGraph& graph) {
    graph_ = &graph;
    ned::SetCurrentEditor(context_);
    ned::Begin("MaterialEditor", ImVec2(0, 0));

    // ノード描画
    for (auto& node : graph.nodes) {
        ned::BeginNode(node->GetId());
        ImGui::Text("%s", node->GetLabel().c_str());
        ImGui::Dummy(ImVec2(8, 4));

        // ノード固有のUIを描画
        node->DrawNodeUI();

        // 入力ピン
        for (auto& pin : node->GetInputs()) {
            ned::BeginPin(pin.id, ned::PinKind::Input);
            ImGui::Text("→ %s", pin.name.c_str());
            ned::EndPin();
        }

        ImGui::SameLine(120);

        // 出力ピン
        ImGui::BeginGroup();
        for (auto& pin : node->GetOutputs()) {
            ned::BeginPin(pin.id, ned::PinKind::Output);
            ImGui::Text("%s →", pin.name.c_str());
            ned::EndPin();
        }
        ImGui::EndGroup();

        ned::EndNode();
    }

    // リンク描画
    for (auto& link : graph.links) {
        ned::Link(link.id, link.startPinId, link.endPinId);
    }

    // 接続操作
    HandleLinkCreation(graph);
    HandleLinkDeletion(graph);

    // 右クリックメニュー
    HandleContextMenu(graph);

    ned::End();
    ned::SetCurrentEditor(nullptr);
}

void MaterialNodeWindow::HandleLinkCreation(MaterialGraph& graph) {
    if (ned::BeginCreate()) {
        ned::PinId startPin, endPin;
        if (ned::QueryNewLink(&startPin, &endPin)) {
            if (startPin && endPin) {
                // 型チェック
                if (CanConnect(graph, static_cast<int>(startPin.Get()), static_cast<int>(endPin.Get()))) {
                    if (ned::AcceptNewItem(ImVec4(0.5f, 1, 0.5f, 1))) {
                        graph.links.push_back({ graph.GetNextId(), static_cast<int>(startPin.Get()), static_cast<int>(endPin.Get())});
                        // グラフ変更を通知
                        dirtyFlag_ = true;
                    }
                } else {
                    // 接続できないなら赤色に表示
                    ned::RejectNewItem(ImVec4(1, 0.3f, 0.3f, 1));
                }
            }
        }
    }
    ned::EndCreate();
}

void MaterialNodeWindow::HandleLinkDeletion(MaterialGraph& graph) {
    if (ned::BeginDelete()) {
        ned::LinkId linkId;
        while (ned::QueryDeletedLink(&linkId)) {
            if (ned::AcceptDeletedItem()) {
                auto it = std::remove_if(
                    graph.links.begin(), graph.links.end(),
                    [&](const Link& l) { return l.id == static_cast<int>(linkId.Get()); }
                );
                graph.links.erase(it, graph.links.end());
                dirtyFlag_ = true;
            }
        }
    }
    ned::EndDelete();
}

void MaterialNodeWindow::HandleContextMenu(MaterialGraph& graph) {
    ned::Suspend();

    if (ned::ShowBackgroundContextMenu()) {
        ImGui::OpenPopup("NodeEditorContext");
    }

    if (ImGui::BeginPopup("NodeEditorContext")) {
        // ノード追加メニュー
        if (ImGui::BeginMenu("AddNode")) {
            for (auto& [name, registerFunc] : registerNode_) {
                // ノードを追加
                if (ImGui::MenuItem(name.c_str())) {
                    registerFunc(graph);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
    ned::Resume();
}

//===========================================
// ヘルパー関数
//===========================================

namespace GameEngine {

    namespace {

        bool CanConnect(const MaterialGraph& graph, int startPinId, int endPinId) {
            const Pin* startPin = graph.FindPin(startPinId);
            const Pin* endPin = graph.FindPin(endPinId);

            // どちらかのピンが見つからなければ接続不可
            if (!startPin || !endPin) return false;

            // 同じノード内のピン同士は接続不可
            if (startPin->parentNodeId == endPin->parentNodeId) return false;

            // Input同士、Output同士は接続不可
            if (startPin->pinKind == endPin->pinKind) return false;

            // 型のチェック
            if (startPin->pinType != endPin->pinType) return false;

            // 入力ピン側に対して、すでに別のリンクが存在している場合は接続不可にする
            int inputPinId = (startPin->pinKind == PinKind::kInput) ? startPinId : endPinId;
            for (const auto& link : graph.links) {
                if (link.endPinId == inputPinId || link.startPinId == inputPinId) {
                    return false;
                }
            }

            return true;
        }
    }
}

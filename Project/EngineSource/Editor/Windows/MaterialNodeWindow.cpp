#include "MaterialNodeWindow.h"
#include <fstream>
#include "NodeSystem/MaterialNode.h"
#include "PSOManager.h"
#include "JsonSerializer.h"
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
    RegisterNode<ColorNode>("Color");
    RegisterNode<TextureSampleNode>("TextureSample");
    RegisterNode<PBROutputNode>("PBROutput");
    RegisterNode<CameraNode>("Camera");
    RegisterNode<LightNode>("Light");

    // 全マテリアルをフォルダからロード
    InitializeAndLoadAllMaterials();
}

void MaterialNodeWindow::Draw() {
    if (!isActive) return;

    ImGui::Begin("MaterialNodeWindow", &isActive);

    DrawMaterialToolbar();

    ImGui::Separator();

    // 選択中のマテリアルがあればノードエディタを描画
    if (currentGraph_) {
        Render(*currentGraph_);
    } else {
        ImGui::TextDisabled("No material active. Click 'New' to create one or select from the list.");
    }

    ImGui::End();
}

void MaterialNodeWindow::Render(MaterialGraph& graph) {
    // 左サイドバー
    ImGui::BeginChild("LeftPanel", ImVec2(leftPanelWidth_, 0), true);

    // ノードパレット
    if (ImGui::CollapsingHeader("NodePalette", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        for (auto& [name, registerFunc] : registerNode_) {
            if (ImGui::Button(name.c_str(), ImVec2(-1, 26))) {
                registerFunc(graph);
                dirtyFlag_ = true;
            }
        }
        ImGui::Spacing();
    }

    // 配置済みノードリスト
    if (ImGui::CollapsingHeader("PlaceDnodes", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        if (graph.nodes.empty()) {
            ImGui::TextDisabled("  No nodes placed.");
        } else {
            for (auto& node : graph.nodes) {
                std::string label = std::format("{} (ID:{})", node->GetLabel(), node->GetId());
                if (ImGui::Selectable(label.c_str())) {
                    ned::SetCurrentEditor(context_);
                    ned::SelectNode(node->GetId());
                    ned::NavigateToSelection();
                }
            }
        }
        ImGui::Spacing();
    }

    // 左サイドバー終了
    ImGui::EndChild();
    ImGui::SameLine();

    // 目立たない細いボタンを境界線として配置
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
    ImGui::Button("##splitter", ImVec2(4.0f, -1));
    ImGui::PopStyleColor(3);

    // バーがドラッグされたら、マウスの移動量に応じて横幅を更新する
    if (ImGui::IsItemActive()) {
        leftPanelWidth_ += ImGui::GetIO().MouseDelta.x;
        // 最小・最大幅を超えないように制限
        if (leftPanelWidth_ < kMinPanelWidth_) { leftPanelWidth_ = kMinPanelWidth_; }
        if (leftPanelWidth_ > kMaxPanelWidth_) { leftPanelWidth_ = kMaxPanelWidth_; }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    ImGui::SameLine();

    // 右の領域
    ImGui::BeginChild("EditorPanel", ImVec2(0, 0), false);

    ned::SetCurrentEditor(context_);
    ned::Begin("MaterialEditor", ImVec2(0, 0));

    // ノード描画
    for (auto& node : graph.nodes) {
        ned::BeginNode(node->GetId());
        ImGui::Text("%s", node->GetLabel().c_str());
        ImGui::Dummy(ImVec2(8, 4));

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

        node->DrawNodeUI();

        ned::EndNode();
    }

    // リンク描画
    for (const auto& link : graph.links) {
        ned::Link(link.id, link.startPinId, link.endPinId);
    }

    HandleLinkCreation(graph);
    HandleLinkDeletion(graph);
    HandleContextMenu(graph);

    ned::End();
    ImGui::EndChild();
}

void MaterialNodeWindow::HandleLinkCreation(MaterialGraph& graph) {
    if (ned::BeginCreate(ImVec4(1, 1, 1, 1), 2.0f)) {
        ned::PinId startPinId, endPinId;
        if (ned::QueryNewLink(&startPinId, &endPinId)) {
            if (CanConnect(graph, static_cast<int>(startPinId.Get()), static_cast<int>(endPinId.Get()))) {
                if (ned::AcceptNewItem(ImVec4(0, 1, 0, 1), 2.0f)) {
                    int sId = static_cast<int>(startPinId.Get());
                    int eId = static_cast<int>(endPinId.Get());

                    graph.links.push_back({ graph.GetNextId(), sId, eId });
                    dirtyFlag_ = true;

                    const Pin* startPin = graph.FindPin(sId);
                    const Pin* endPin = graph.FindPin(eId);
                    if (startPin && endPin) {
                        IMaterialNode* startNode = graph.FindNode(startPin->parentNodeId);
                        IMaterialNode* endNode = graph.FindNode(endPin->parentNodeId);

                        // startNodeにはendPinの型を、endNodeにはstartPinの型を通知
                        if (startNode) { startNode->OnConnectTypePropagate(endPin->pinType); }
                        if (endNode) { endNode->OnConnectTypePropagate(startPin->pinType); }
                    }
                }
            } else {
                ned::RejectNewItem(ImVec4(1, 0.3f, 0.3f, 1));
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

void MaterialNodeWindow::DrawMaterialToolbar() {
    // マテリアル選択コンボボックス
    std::string previewName = currentMaterialName_.empty() ? "Select Material..." : currentMaterialName_;
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::BeginCombo("##MaterialSelect", previewName.c_str())) {
        for (const auto& matName : materialList_) {
            bool isSelected = (currentMaterialName_ == matName);
            if (ImGui::Selectable(matName.c_str(), isSelected)) {
                SelectMaterial(matName);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();

    // 新規作成ボタン
    if (ImGui::Button("New")) {
        ImGui::OpenPopup("NewMaterialPopup");
    }

    // 新規作成ポップアップ
    if (ImGui::BeginPopup("NewMaterialPopup")) {
        static char newMatName[64] = "NewMaterial";
        ImGui::InputText("Name", newMatName, IM_ARRAYSIZE(newMatName));
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            CreateNewMaterial(newMatName);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // 保存ボタン
    if (ImGui::Button("Save") && currentGraph_) {
        SaveCurrentMaterial();
    }

    // 未保存の変更マーク
    if (dirtyFlag_) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "*");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Unsaved changes in this material");
        }
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // HLSL生成ボタン
    if (currentGraph_) {
        if (ImGui::Button("CreateHLSL")) {
            std::wstring wName(currentMaterialName_.begin(), currentMaterialName_.end());
            psoManager_->RegisterPSO(*currentGraph_, wName.c_str());
        }
    }
}

void MaterialNodeWindow::InitializeAndLoadAllMaterials() {
    // JsonSerializerを使って、フォルダ内のJSONを一括読み込みしてキャッシュに登録
    JsonSerializer::LoadDirectory(kDirectoryPath, [this](const std::string& stem, const nlohmann::json& root) {
        MaterialGraph graph;

        // 実際の復元処理を行う

        graphData_[stem] = std::move(graph);
    });

    UpdateMaterialList();

    // 1つ以上マテリアルが存在していれば、最初のアセットを選択状態にする
    if (!materialList_.empty()) {
        SelectMaterial(materialList_[0]);
    }
}

void MaterialNodeWindow::UpdateMaterialList() {
    materialList_.clear();
    for (const auto& [name, graph] : graphData_) {
        materialList_.push_back(name);
    }
}

void MaterialNodeWindow::CreateNewMaterial(const std::string& name) {
    if (name.empty()) return;

    // 同名マテリアルがすでにあれば上書きを避けるか、適宜リネーム
    std::string finalName = name;
    int count = 1;
    while (graphData_.find(finalName) != graphData_.end()) {
        finalName = name + "_" + std::to_string(count++);
    }

    // メモリ上に新規グラフを作成
    graphData_[finalName] = MaterialGraph();

    // 最初からデフォルトで配置しておきたいノードがあればここで登録
    // graphData_[finalName].nodes.push_back(std::make_unique<PBROutputNode>(graphData_[finalName]));

    UpdateMaterialList();
    SelectMaterial(finalName);

    // 新規作成と同時にファイルも一度ディスクに書き出す
    SaveCurrentMaterial();
}

void MaterialNodeWindow::SelectMaterial(const std::string& name) {
    if (graphData_.find(name) != graphData_.end()) {
        currentMaterialName_ = name;
        currentGraph_ = &graphData_[name];
        dirtyFlag_ = false;
    }
}

void MaterialNodeWindow::SaveCurrentMaterial() {
    if (!currentGraph_ || currentMaterialName_.empty()) return;

    // グラフをjsonオブジェクトにシリアライズする
    nlohmann::json root = nlohmann::json::object();

    // 実際の保存処理

    // 暫定プレースホルダー
    root["materialName"] = currentMaterialName_;

    // ファイルへ書き出し
    std::string filePath = kDirectoryPath + currentMaterialName_ + ".json";
    JsonSerializer::SaveToFile(filePath, root);

    dirtyFlag_ = false;
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
            if (!startPin || !endPin) { return false; }
            // 同じノード内のピン同士は接続不可
            if (startPin->parentNodeId == endPin->parentNodeId) { return false; }
            // Input同士、Output同士は接続不可
            if (startPin->pinKind == endPin->pinKind) { return false; }

            // 型のチェック
            if (startPin->pinType != endPin->pinType) {
                IMaterialNode* startNode = graph.FindNode(startPin->parentNodeId);
                IMaterialNode* endNode = graph.FindNode(endPin->parentNodeId);

                // 数値型かどうかを判定するラムダ
                auto isNumeric = [](PinType t) {
                    return t == PinType::kFloat || t == PinType::kFloat2 ||
                        t == PinType::kFloat3 || t == PinType::kFloat4;
                };

                // どちらかのノードが可変型であり、かつお互いが数値型であれば接続を許可
                bool startDynamic = startNode && startNode->IsVariableType() && isNumeric(endPin->pinType);
                bool endDynamic = endNode && endNode->IsVariableType() && isNumeric(startPin->pinType);

                if (!startDynamic && !endDynamic) {
                    return false;
                }
            }

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

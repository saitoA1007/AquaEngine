#include "pch.h"
#include "PostEffectWindow.h"
#include "LogManager.h"
#include "PostProcess/PostEffectManager.h"

using namespace GameEngine;
namespace ed = ax::NodeEditor;

PostEffectWindow::PostEffectWindow(PostEffectManager* postEffectManager) {
    assert(postEffectManager != nullptr);
    postEffectManager_ = postEffectManager;

    ed::Config config;
    // 位置はPostEffectNode::posで管理する
    config.SettingsFile = nullptr;
    context_ = ed::CreateEditor(&config);
}

PostEffectWindow::~PostEffectWindow() {
    if (context_) { ed::DestroyEditor(context_); }
}

void PostEffectWindow::Draw() {
    ImGui::Begin("PostEffect", &isActive);
    if (!postEffectManager_) { ImGui::TextUnformatted("PostEffectManager is not set."); ImGui::End(); return; }

    PostEffectGraph& graph = postEffectManager_->GetGraph();

    // ツールバー
    if (ImGui::Button("Reset")) {
        postEffectManager_->ResetGraph();
        applyPositions_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Fit")) { ed::SetCurrentEditor(context_); ed::NavigateToContent(); ed::SetCurrentEditor(nullptr); }

    ed::SetCurrentEditor(context_);
    ed::Begin("PostEffectGraph");

    // ノード描画
    for (auto& node : graph.nodes) {
        if (applyPositions_) { ed::SetNodePosition(node->id, ImVec2(node->pos.x, node->pos.y)); }

        ed::BeginNode(node->id);
        ImGui::PushID(node->id);

        ImGui::TextUnformatted(node->label.c_str());

        if (node->kind == PostEffectNodeKind::kEffect) {
            // 有効/無効
            bool active = node->effect->IsActive();
            if (ImGui::Checkbox("Active", &active)) { node->effect->SetIsActive(active); }

            ImGui::PushItemWidth(120.0f);
            node->effect->DrawParamUI();
            ImGui::PopItemWidth();
        }

        // 入力ピン
        for (const Pin& pin : node->inputs) {
            ed::BeginPin(pin.id, ed::PinKind::Input);
            ImGui::Text("-> %s", pin.name.c_str());
            ed::EndPin();
        }
        // 出力ピン
        if (node->kind != PostEffectNodeKind::kOutput) {
            ed::BeginPin(node->output.id, ed::PinKind::Output);
            ImGui::Text("%s ->", node->output.name.c_str());
            ed::EndPin();
        }

        ImGui::PopID();
        ed::EndNode();

        // ユーザーが動かした位置を書き戻す
        if (!applyPositions_) {
            ImVec2 p = ed::GetNodePosition(node->id);
            node->pos = { p.x, p.y };
        }
    }
    applyPositions_ = false;

    // リンク描画
    for (const Link& link : graph.links) {
        ed::Link(link.id, link.startPinId, link.endPinId);
    }

    // リンク作成
    if (ed::BeginCreate()) {
        ed::PinId a, b;
        if (ed::QueryNewLink(&a, &b) && a && b) {
            int startId = static_cast<int>(a.Get());
            int endId = static_cast<int>(b.Get());

            // 入力→出力の順でドラッグされた場合は入れ替えて正規化する
            const Pin* pa = graph.FindPin(startId);
            if (pa && pa->pinKind == PinKind::kInput) { std::swap(startId, endId); }

            if (graph.CanConnect(startId, endId)) {
                if (ed::AcceptNewItem()) { graph.AddLink(startId, endId); }
            } else {
                ed::RejectNewItem(ImColor(255, 80, 80), 2.0f);   // 繋げない場合は赤
            }
        }
    }
    ed::EndCreate();

    // 削除
    if (ed::BeginDelete()) {
        ed::LinkId linkId;
        while (ed::QueryDeletedLink(&linkId)) {
            if (ed::AcceptDeletedItem()) { graph.RemoveLink(static_cast<int>(linkId.Get())); }
        }
        ed::NodeId nodeId;
        while (ed::QueryDeletedNode(&nodeId)) {
            PostEffectNode* n = graph.FindNode(static_cast<int>(nodeId.Get()));
            // Scene、Outputは消せない
            if (n && n->kind == PostEffectNodeKind::kEffect && ed::AcceptDeletedItem()) {
                graph.RemoveNode(n->id);
            } else {
                ed::RejectDeletedItem();
            }
        }
    }
    ed::EndDelete();

    // 右クリックメニュー
    ImVec2 openPos = ImGui::GetMousePos();
    ed::Suspend();
    if (ed::ShowBackgroundContextMenu()) {
        popupCanvasPos_ = ed::ScreenToCanvas(openPos);
        ImGui::OpenPopup("AddPostEffectNode");
    }
    if (ImGui::BeginPopup("AddPostEffectNode")) {
        ImGui::TextUnformatted("Add Effect");
        ImGui::Separator();

        bool any = false;
        for (const auto& [passName, effect] : postEffectManager_->GetEffects()) {
            // すでにグラフ上にあるエフェクトは追加できないように
            bool exists = false;
            for (auto& n : graph.nodes) {
                if (n->effect == effect.get()) { exists = true; break; }
            }
            if (exists) { continue; }

            any = true;
            if (ImGui::MenuItem(effect->GetDisplayName())) {
                PostEffectNode* added = graph.AddEffectNode(passName, effect.get());
                added->pos = { popupCanvasPos_.x, popupCanvasPos_.y };
                ed::SetNodePosition(added->id, popupCanvasPos_);
            }
        }
        if (!any) { ImGui::TextDisabled("(no effects available)"); }
        ImGui::EndPopup();
    }
    ed::Resume();

    ed::End();
    ed::SetCurrentEditor(nullptr);
    ImGui::End();
}
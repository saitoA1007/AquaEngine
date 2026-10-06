#include "pch.h"
#include "PostEffectWindow.h"
#include "LogManager.h"
#include "PostProcess/PostEffectManager.h"
#include "TextureManager.h"
#include "RenderPassController.h"

using namespace GameEngine;
namespace ned = ax::NodeEditor;

namespace {
    static ImTextureID ToImTex(TextureManager* tm, uint32_t handle) {
        D3D12_GPU_DESCRIPTOR_HANDLE h = tm->GetTextureSrvHandlesGPU(handle);
        return (ImTextureID)h.ptr;
    }

    // ヘッダー画像に乗算する色
    static ImU32 HeaderTint(PostEffectNodeKind kind, bool active) {
        if (!active) { return IM_COL32(110, 110, 115, 255); }
        switch (kind) {
        case PostEffectNodeKind::kScene:  return IM_COL32(130, 220, 150, 255);
        case PostEffectNodeKind::kOutput: return IM_COL32(248, 140, 120, 255);
        default:                          return IM_COL32(128, 195, 248, 255);
        }
    }
}

PostEffectWindow::PostEffectWindow(PostEffectManager* postEffectManager, TextureManager* textureManager, RenderPassController* renderPassController) {
    assert(postEffectManager != nullptr);
    assert(textureManager != nullptr);
    assert(renderPassController != nullptr);
    postEffectManager_ = postEffectManager;
    textureManager_ = textureManager;
    renderPassController_ = renderPassController;

    // 画像を取得
    headerBgHandle_ = textureManager_->GetHandleByName("BlueprintBackground.png");
    iconRestoreHandle_ = textureManager_->GetHandleByName("ic_restore_white_24dp.png");
    iconSaveHandle_ = textureManager_->GetHandleByName("ic_save_white_24dp.png");

    ned::Config config;
    // 位置はPostEffectNode::posで管理する
    config.SettingsFile = nullptr;
    context_ = ned::CreateEditor(&config);

    ned::SetCurrentEditor(context_);
    NodeUI::ApplyEditorStyle();
    ned::SetCurrentEditor(nullptr);

    nodeStyle_.width = 240.0f;
    nodeStyle_.iconSize = 24.0f;
    nodeStyle_.headerTexSize = ImVec2(64.0f, 64.0f);
    nodeStyle_.headerTexture = headerBgHandle_ ? ToImTex(textureManager_, headerBgHandle_) : ImTextureID{};
}

PostEffectWindow::~PostEffectWindow() {
    if (context_) { ned::DestroyEditor(context_); }
}

void PostEffectWindow::Draw() {
    ImGui::Begin("PostEffect", &isActive);
    if (!postEffectManager_) { ImGui::TextUnformatted("PostEffectManager is not set."); ImGui::End(); return; }

    PostEffectGraph& graph = postEffectManager_->GetGraph();

    // ツールバー
    if (ImGui::ImageButton("##reset", ToImTex(textureManager_, iconRestoreHandle_), ImVec2(16, 16))) {
        postEffectManager_->ResetGraph();
        applyPositions_ = true;
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Reset"); }

    ImGui::SameLine();
    if (ImGui::Button("Fit")) {
        ned::SetCurrentEditor(context_);
        ned::NavigateToContent();
        ned::SetCurrentEditor(nullptr);
    }

    ImGui::SameLine();
    if (ImGui::ImageButton("##save", ToImTex(textureManager_, iconSaveHandle_), ImVec2(16, 16))) {
        // 保存処理(後でグラフをJSONに書き出すなど)
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Save"); }
    ImGui::SameLine();
    ImGui::Checkbox("Preview", &showPreview_);

    ned::SetCurrentEditor(context_);
    ned::Begin("PostEffectGraph");

    // ノード描画
    for (auto& node : graph.nodes) {
        if (applyPositions_) { ned::SetNodePosition(node->id, ImVec2(node->pos.x, node->pos.y)); }

        DrawNode(graph, *node);

        if (!applyPositions_) {
            ImVec2 p = ned::GetNodePosition(node->id);
            node->pos = { p.x, p.y };
        }
    }
    applyPositions_ = false;

    // リンク描画
    {
        // Outputに到達する経路上のノードを集める
        std::unordered_set<int> livePath;
        std::vector<int> order;
        if (graph.BuildOrder(order)) { livePath.insert(order.begin(), order.end()); }

        for (const Link& link : graph.links) {
            const Pin* s = graph.FindPin(link.startPinId);
            const Pin* e = graph.FindPin(link.endPinId);

            // 両端のノードが経路上にあり、有効なエフェクトなら流す
            bool flowing = false;
            if (s && e && livePath.contains(s->parentNodeId) && livePath.contains(e->parentNodeId)) {
                const PostEffectNode* from = graph.FindNode(s->parentNodeId);
                const PostEffectNode* to = graph.FindNode(e->parentNodeId);
                auto isOn = [](const PostEffectNode* n) {
                    return n && (n->kind != PostEffectNodeKind::kEffect || n->effect->IsActive());
                    };
                flowing = isOn(from) && isOn(to);
            }

            ImColor color = flowing ? ImColor(120, 200, 255, 230) : ImColor(110, 110, 120, 160);
            ned::Link(link.id, link.startPinId, link.endPinId, color, flowing ? 2.5f : 1.5f);

            if (flowing) { ned::Flow(link.id, ned::FlowDirection::Forward); }
        }
    }

    // リンク作成
    if (ned::BeginCreate()) {
        ned::PinId a, b;
        if (ned::QueryNewLink(&a, &b) && a && b) {
            int startId = static_cast<int>(a.Get());
            int endId = static_cast<int>(b.Get());

            // 入力→出力の順でドラッグされた場合は入れ替えて正規化する
            const Pin* pa = graph.FindPin(startId);
            if (pa && pa->pinKind == PinKind::kInput) { std::swap(startId, endId); }

            if (graph.CanConnect(startId, endId)) {
                if (ned::AcceptNewItem()) { graph.AddLink(startId, endId); }
            } else {
                ned::RejectNewItem(ImColor(255, 80, 80), 2.0f);   // 繋げない場合は赤
            }
        }
    }
    ned::EndCreate();

    // 削除
    if (ned::BeginDelete()) {
        ned::LinkId linkId;
        while (ned::QueryDeletedLink(&linkId)) {
            if (ned::AcceptDeletedItem()) { graph.RemoveLink(static_cast<int>(linkId.Get())); }
        }
        ned::NodeId nodeId;
        while (ned::QueryDeletedNode(&nodeId)) {
            PostEffectNode* n = graph.FindNode(static_cast<int>(nodeId.Get()));
            // Scene、Outputは消せない
            if (n && n->kind == PostEffectNodeKind::kEffect && ned::AcceptDeletedItem()) {
                graph.RemoveNode(n->id);
            } else {
                ned::RejectDeletedItem();
            }
        }
    }
    ned::EndDelete();

    // 右クリックメニュー
    ImVec2 openPos = ImGui::GetMousePos();
    ned::Suspend();
    if (ned::ShowBackgroundContextMenu()) {
        popupCanvasPos_ = ned::ScreenToCanvas(openPos);
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
                ned::SetNodePosition(added->id, popupCanvasPos_);
            }
        }
        if (!any) { ImGui::TextDisabled("(no effects available)"); }
        ImGui::EndPopup();
    }
    ned::Resume();

    ned::End();
    ned::SetCurrentEditor(nullptr);
    ImGui::End();
}

void PostEffectWindow::DrawNode(PostEffectGraph& graph, PostEffectNode& node) {
    
    const bool isEffect = node.kind == PostEffectNodeKind::kEffect;
    const bool active = !isEffect || node.effect->IsActive();
    const Pin* mainIn = node.inputs.empty() ? nullptr : &node.inputs[0];
    const bool hasOut = node.kind != PostEffectNodeKind::kOutput;
    const ImU32 flowColor = IM_COL32_WHITE;

    NodeUI::NodeBuilder nb(nodeStyle_);
    nb.Begin(node.id);

    // ヘッダー
    nb.BeginHeader(node.label, HeaderTint(node.kind, active));
    if (isEffect) {
        ImGui::SameLine(0, 16);
        bool a = node.effect->IsActive();
        if (ImGui::Checkbox("##active", &a)) { node.effect->SetIsActive(a); }
    }
    nb.EndHeader();

    // メインの入出力
    if (mainIn && hasOut) {
        nb.PinPair(ned::PinId(mainIn->id), "", NodeUI::PinIconType::Flow, flowColor, graph.IsPinLinked(mainIn->id),
            ned::PinId(node.output.id), "", NodeUI::PinIconType::Flow, flowColor, graph.IsPinLinked(node.output.id));
    } else if (mainIn) {
        nb.InputPin(ned::PinId(mainIn->id), "", NodeUI::PinIconType::Flow, flowColor, graph.IsPinLinked(mainIn->id));
    } else if (hasOut) {
        nb.OutputPin(ned::PinId(node.output.id), "", NodeUI::PinIconType::Flow, flowColor, graph.IsPinLinked(node.output.id));
    }

    // 2番目以降の入力
    for (size_t i = 1; i < node.inputs.size(); ++i) {
        const Pin& pin = node.inputs[i];
        nb.InputPin(ned::PinId(pin.id), pin.name, NodeUI::PinIconType::Data, NodeUI::PinTypeColor(pin.pinType), graph.IsPinLinked(pin.id));
    }

    // パラメータ
    if (isEffect) {
        nb.BeginContent();
        ImGui::BeginDisabled(!active);
        ImGui::PushItemWidth(120.0f);
        node.effect->DrawParamUI();
        ImGui::PopItemWidth();
        ImGui::EndDisabled();
    }

    // 中間描画結果のプレビュー
    if (showPreview_) {
        ImTextureID tex{};
        const std::string pass = GetPreviewPassName(graph, node);
        if (!pass.empty()) { tex = (ImTextureID)renderPassController_->GetSrvHandle(pass).ptr; }
        nb.Preview(tex, !active);
    }

    nb.End();
}

std::string PostEffectWindow::GetPreviewPassName(const PostEffectGraph& graph, const PostEffectNode& node) const {
    switch (node.kind) {
        // 元のシーン画像
    case PostEffectNodeKind::kScene:
        return renderPassController_->GetSceneFinalPass();

        // 各エフェクトのパス
    case PostEffectNodeKind::kEffect:
        return node.passName;    

    case PostEffectNodeKind::kOutput: {
        // Outputは自前の画像を持たないので、つながっている上流ノードを表示する
        if (node.inputs.empty()) { return {}; }
        const Link* l = graph.FindLinkToPin(node.inputs[0].id);
        if (!l) { return {}; }
        const Pin* s = graph.FindPin(l->startPinId);
        const PostEffectNode* up = s ? graph.FindNode(s->parentNodeId) : nullptr;
        return up ? GetPreviewPassName(graph, *up) : std::string{};
    }
    }
    return {};
}
#include "pch.h"
#include "PostEffectWindow.h"
#include "LogManager.h"
#include "PostProcess/PostEffectManager.h"
#include "TextureManager.h"

using namespace GameEngine;
namespace ed = ax::NodeEditor;

namespace {
    constexpr float kNodeWidth = 240.0f;   // ノード内容の固定幅
    constexpr float kIconSize = 24.0f;
    constexpr float kHeaderTexW = 64.0f;
    constexpr float kHeaderTexH = 64.0f;
    constexpr ImU32 kTexturePinColor = IM_COL32(51, 150, 215, 255);

    // ヘッダー画像に乗算する色
    static ImU32 HeaderTint(PostEffectNodeKind kind, bool active) {
        if (!active) { return IM_COL32(110, 110, 115, 255); }
        switch (kind) {
        case PostEffectNodeKind::kScene:  return IM_COL32(130, 220, 150, 255);
        case PostEffectNodeKind::kOutput: return IM_COL32(248, 140, 120, 255);
        default:                          return IM_COL32(128, 195, 248, 255);
        }
    }

    static ImU32 HeaderColor(PostEffectNodeKind kind, bool active) {
        ImU32 c = kind == PostEffectNodeKind::kScene ? IM_COL32(40, 120, 70, 255)
            : kind == PostEffectNodeKind::kOutput ? IM_COL32(150, 60, 50, 255)
            : IM_COL32(50, 90, 160, 255);
        return active ? c : IM_COL32(70, 70, 74, 255);   // 無効エフェクトはグレー
    }

    static void DrawPinIcon(bool connected, ImU32 color, float size = 14.0f) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImVec2 c(p.x + size * 0.5f, p.y + size * 0.5f);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (connected) { dl->AddCircleFilled(c, size * 0.32f, color, 16); } else { dl->AddCircle(c, size * 0.32f, color, 16, 2.0f); }
        ImGui::Dummy(ImVec2(size, size));
    }

    static ImTextureID ToImTex(TextureManager* tm, uint32_t handle) {
        D3D12_GPU_DESCRIPTOR_HANDLE h = tm->GetTextureSrvHandlesGPU(handle);
        return (ImTextureID)h.ptr;
    }

    // 上段のD字アイコン
    static void DrawFlowIcon(bool connected, ImU32 color = IM_COL32(255, 255, 255, 255)) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        const float s = kIconSize;
        const float l = p.x + s * 0.20f, mid = p.x + s * 0.55f, r = p.x + s * 0.90f;
        const float t = p.y + s * 0.20f, b = p.y + s * 0.80f, cy = p.y + s * 0.50f;
        ImVec2 pts[5] = { {l, t}, {mid, t}, {r, cy}, {mid, b}, {l, b} };
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (connected) { dl->AddConvexPolyFilled(pts, 5, color); }
        dl->AddPolyline(pts, 5, color, ImDrawFlags_Closed, 2.0f);
        ImGui::Dummy(ImVec2(s, s));
    }

    // 丸+右向き三角のデータピンアイコン
    static void DrawDataPinIcon(bool connected, ImU32 color) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        const float s = kIconSize;
        ImVec2 c(p.x + s * 0.42f, p.y + s * 0.5f);
        const float r = s * 0.22f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (connected) { dl->AddCircleFilled(c, r, color, 24); } else { dl->AddCircle(c, r, color, 24, 2.0f); }
        const float tx = c.x + r + 3.0f;
        dl->AddTriangleFilled(ImVec2(tx, c.y - 3.5f), ImVec2(tx, c.y + 3.5f), ImVec2(tx + 5.0f, c.y), color);
        ImGui::Dummy(ImVec2(s, s));
    }
}

PostEffectWindow::PostEffectWindow(PostEffectManager* postEffectManager, TextureManager* textureManager, RenderPassController* renderPassController) {
    assert(postEffectManager != nullptr);
    assert(textureManager != nullptr);
    assert(renderPassController != nullptr);
    postEffectManager_ = postEffectManager;
    textureManager_ = textureManager;
    renderPassController_ = renderPassController;

    textureManager_->RegisterTexture("EngineSource/Resources/Textures/BlueprintBackground.png");
    textureManager_->RegisterTexture("EngineSource/Resources/Textures/ic_restore_white_24dp.png");
    textureManager_->RegisterTexture("EngineSource/Resources/Textures/ic_save_white_24dp.png");

    // 画像を取得
    headerBgHandle_ = textureManager_->GetHandleByName("BlueprintBackground.png");
    iconRestoreHandle_ = textureManager_->GetHandleByName("ic_restore_white_24dp.png");
    iconSaveHandle_ = textureManager_->GetHandleByName("ic_save_white_24dp.png");

    ed::Config config;
    // 位置はPostEffectNode::posで管理する
    config.SettingsFile = nullptr;
    context_ = ed::CreateEditor(&config);

    ed::SetCurrentEditor(context_);
    auto& style = ed::GetStyle();
    style.NodePadding = ImVec4(8, 4, 8, 8);
    style.NodeRounding = 12.0f;
    style.NodeBorderWidth = 1.0f;
    style.HoveredNodeBorderWidth = 3.0f;
    style.SelectedNodeBorderWidth = 3.0f;
    style.LinkStrength = 120.0f;               // ベジェの膨らみ
    style.FlowMarkerDistance = 30.0f;
    style.FlowSpeed = 150.0f;

    style.FlowDuration = 2.0f;   // 1回のFlowの継続秒数
    style.Colors[ed::StyleColor_Flow] = ImColor(255, 200, 80, 255);   // 流れる線の色
    style.Colors[ed::StyleColor_FlowMarker] = ImColor(255, 230, 150, 255);  // 粒の色

    style.Colors[ed::StyleColor_Bg] = ImColor(26, 26, 28, 255);
    style.Colors[ed::StyleColor_Grid] = ImColor(255, 255, 255, 18);
    style.Colors[ed::StyleColor_NodeBg] = ImColor(32, 32, 32, 220);
    style.Colors[ed::StyleColor_NodeBorder] = ImColor(255, 255, 255, 96);
    style.Colors[ed::StyleColor_HovNodeBorder] = ImColor(255, 255, 255, 160);
    style.Colors[ed::StyleColor_SelNodeBorder] = ImColor(255, 170, 40, 255);   // UEの選択オレンジ
    style.Colors[ed::StyleColor_PinRect] = ImColor(0, 0, 0, 0);
    style.Colors[ed::StyleColor_PinRectBorder] = ImColor(0, 0, 0, 0);          // ピン自体の枠は消す

    ed::SetCurrentEditor(nullptr);
}

PostEffectWindow::~PostEffectWindow() {
    if (context_) { ed::DestroyEditor(context_); }
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
    if (ImGui::Button("Fit")) { /* 既存のまま */ }

    ImGui::SameLine();
    if (ImGui::ImageButton("##save", ToImTex(textureManager_, iconSaveHandle_), ImVec2(16, 16))) {
        // 保存処理(後でグラフをJSONに書き出すなど)
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Save"); }

    ed::SetCurrentEditor(context_);
    ed::Begin("PostEffectGraph");

    const ImU32 pinColor = IM_COL32(120, 200, 255, 255);

    // ノード描画
    for (auto& node : graph.nodes) {
        if (applyPositions_) { ed::SetNodePosition(node->id, ImVec2(node->pos.x, node->pos.y)); }

        const bool active = (node->kind != PostEffectNodeKind::kEffect) || node->effect->IsActive();
        const bool hasOut = node->kind != PostEffectNodeKind::kOutput;
        const Pin* mainIn = node->inputs.empty() ? nullptr : &node->inputs[0];
        const float spacing = ImGui::GetStyle().ItemSpacing.x;

        ed::BeginNode(node->id);
        ImGui::PushID(node->id);

        // ヘッダー
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(kNodeWidth, 2));
        ImGui::TextUnformatted(node->label.c_str());
        if (node->kind == PostEffectNodeKind::kEffect) {
            ImGui::SameLine(0, 16);
            bool a = node->effect->IsActive();
            if (ImGui::Checkbox("##active", &a)) { node->effect->SetIsActive(a); }
        }
        ImGui::Dummy(ImVec2(0, 2));
        ImGui::EndGroup();
        const float headerBottom = ImGui::GetItemRectMax().y;

        ImGui::Spacing();

        // メイン入力(左) と 出力(右)
        if (mainIn) {
            ed::BeginPin(mainIn->id, ed::PinKind::Input);
            ed::PinPivotAlignment(ImVec2(0.0f, 0.5f));
            ed::PinPivotSize(ImVec2(0, 0));
            DrawFlowIcon(graph.IsPinLinked(mainIn->id));
            ed::EndPin();
            if (hasOut) { ImGui::SameLine(); }
        }
        if (hasOut) {
            const int n = mainIn ? 2 : 1;   // 行に並ぶアイコン数
            ImGui::Dummy(ImVec2(kNodeWidth - (kIconSize + spacing) * n, kIconSize));  // 右寄せ用の空き
            ImGui::SameLine();
            ed::BeginPin(node->output.id, ed::PinKind::Output);
            ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
            ed::PinPivotSize(ImVec2(0, 0));
            DrawFlowIcon(graph.IsPinLinked(node->output.id));
            ed::EndPin();
        }

        // 2番目以降の入力ピン(丸+三角+名前)
        for (size_t i = 1; i < node->inputs.size(); ++i) {
            const Pin& pin = node->inputs[i];
            ed::BeginPin(pin.id, ed::PinKind::Input);
            ed::PinPivotAlignment(ImVec2(0.0f, 0.5f));
            ed::PinPivotSize(ImVec2(0, 0));
            DrawDataPinIcon(graph.IsPinLinked(pin.id), kTexturePinColor);
            ImGui::SameLine();
            ImGui::TextUnformatted(pin.name.c_str());
            ed::EndPin();
        }

        // パラメータ
        float sepY = -1.0f;
        if (node->kind == PostEffectNodeKind::kEffect) {
            ImGui::Spacing();
            sepY = ImGui::GetCursorScreenPos().y;
            ImGui::Dummy(ImVec2(0, 4));

            ImGui::BeginDisabled(!active);
            ImGui::PushItemWidth(120.0f);
            node->effect->DrawParamUI();
            ImGui::PopItemWidth();
            ImGui::EndDisabled();
        }

        ImGui::PopID();
        ed::EndNode();

        // ヘッダー画像 + 区切り線
        {
            ImVec2 pos = ed::GetNodePosition(node->id);
            ImVec2 size = ed::GetNodeSize(node->id);
            auto& st = ed::GetStyle();
            ImDrawList* bg = ed::GetNodeBackgroundDrawList(node->id);

            const float half = st.NodeBorderWidth * 0.5f;
            ImVec2 hMin(pos.x + half, pos.y + half);
            ImVec2 hMax(pos.x + size.x - half, headerBottom + 4.0f);
            ImU32  tint = HeaderTint(node->kind, active);

            if (headerBgHandle_ != 0) {
                // 画像をタイル状に繰り返して貼る(Blueprintsサンプルと同じUV)
                ImVec2 uv((hMax.x - hMin.x) / (4.0f * kHeaderTexW),
                    (hMax.y - hMin.y) / (4.0f * kHeaderTexH));
                bg->AddImageRounded(ToImTex(textureManager_, headerBgHandle_),
                    hMin, hMax, ImVec2(0, 0), uv, tint,
                    st.NodeRounding - 1.0f, ImDrawFlags_RoundCornersTop);
            } else {
                bg->AddRectFilled(hMin, hMax, tint, st.NodeRounding - 1.0f, ImDrawFlags_RoundCornersTop);
            }
            bg->AddLine(ImVec2(hMin.x, hMax.y), ImVec2(hMax.x, hMax.y), IM_COL32(255, 255, 255, 40), 1.0f);

            if (sepY >= 0.0f) {
                bg->AddLine(ImVec2(pos.x + 8.0f, sepY + 2.0f),
                    ImVec2(pos.x + size.x - 8.0f, sepY + 2.0f),
                    IM_COL32(255, 255, 255, 30), 1.0f);
            }
        }

        if (!applyPositions_) {
            ImVec2 p = ed::GetNodePosition(node->id);
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
            ed::Link(link.id, link.startPinId, link.endPinId, color, flowing ? 2.5f : 1.5f);

            if (flowing) { ed::Flow(link.id, ed::FlowDirection::Forward); }
        }
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
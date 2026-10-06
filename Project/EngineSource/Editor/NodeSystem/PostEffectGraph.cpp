#include "pch.h"
#include "PostEffectGraph.h"

using namespace GameEngine;

PostEffectNode* PostEffectGraph::FindNode(int nodeId) const {
    for (auto& n : nodes) { 
        if (n->id == nodeId) {
            return n.get(); 
        }
    }
    return nullptr;
}

PostEffectNode* PostEffectGraph::FindNodeByPin(int pinId) const {
    for (auto& n : nodes) {
        if (n->kind != PostEffectNodeKind::kOutput && n->output.id == pinId) { return n.get(); }
        for (auto& p : n->inputs) {
            if (p.id == pinId) {
                return n.get(); 
            } 
        }
    }
    return nullptr;
}

PostEffectNode* PostEffectGraph::FindOutputNode() const {
    for (auto& n : nodes) { 
        if (n->kind == PostEffectNodeKind::kOutput) { 
            return n.get(); 
        } 
    }
    return nullptr;
}

const Pin* PostEffectGraph::FindPin(int pinId) const {
    for (auto& n : nodes) {
        if (n->kind != PostEffectNodeKind::kOutput && n->output.id == pinId) { return &n->output; }
        for (auto& p : n->inputs) { 
            if (p.id == pinId) { return &p; 
            } 
        }
    }
    return nullptr;
}

const Link* PostEffectGraph::FindLinkToPin(int endPinId) const {
    for (auto& l : links) {
        if (l.endPinId == endPinId) {
            return &l; 
        } 
    }
    return nullptr;
}

PostEffectNode* PostEffectGraph::AddSceneNode() {
    auto n = std::make_unique<PostEffectNode>();
    n->id = GetNextId();
    n->kind = PostEffectNodeKind::kScene;
    n->label = "Scene";
    n->output = { GetNextId(), "Scene", PinType::kTexture2D, PinKind::kOutput, n->id };
    nodes.push_back(std::move(n));
    dirty = true;
    return nodes.back().get();
}

PostEffectNode* PostEffectGraph::AddOutputNode() {
    auto n = std::make_unique<PostEffectNode>();
    n->id = GetNextId();
    n->kind = PostEffectNodeKind::kOutput;
    n->label = "Output";
    n->inputs.push_back({ GetNextId(), "Result", PinType::kTexture2D, PinKind::kInput, n->id });
    nodes.push_back(std::move(n));
    dirty = true;
    return nodes.back().get();
}

PostEffectNode* PostEffectGraph::AddEffectNode(const std::string& passName, IPostEffect* effect) {
    auto n = std::make_unique<PostEffectNode>();
    n->id = GetNextId();
    n->kind = PostEffectNodeKind::kEffect;
    n->label = effect->GetDisplayName();
    n->passName = passName;
    n->effect = effect;
    for (uint32_t i = 0; i < effect->GetInputCount(); ++i) {
        n->inputs.push_back({ GetNextId(), effect->GetInputName(i), PinType::kTexture2D, PinKind::kInput, n->id });
    }
    n->output = { GetNextId(), "Out", PinType::kTexture2D, PinKind::kOutput, n->id };
    nodes.push_back(std::move(n));
    dirty = true;
    return nodes.back().get();
}

int PostEffectGraph::AddLink(int startPinId, int endPinId) {
    // 入力ピンへのリンクは1本だけ
    std::erase_if(links, [&](const Link& l) { return l.endPinId == endPinId; });
    int id = GetNextId();
    links.push_back({ id, startPinId, endPinId });
    dirty = true;
    return id;
}

void PostEffectGraph::RemoveLink(int linkId) {
    std::erase_if(links, [&](const Link& l) { return l.id == linkId; });
    dirty = true;
}

void PostEffectGraph::RemoveNode(int nodeId) {
    PostEffectNode* node = FindNode(nodeId);
    if (!node || node->kind != PostEffectNodeKind::kEffect) { return; }

    // そのノードのピンに繋がるリンクを全部消す
    std::erase_if(links, [&](const Link& l) {
        const Pin* s = FindPin(l.startPinId);
        const Pin* e = FindPin(l.endPinId);
        return (s && s->parentNodeId == nodeId) || (e && e->parentNodeId == nodeId);
        });
    std::erase_if(nodes, [&](const auto& n) { return n->id == nodeId; });
    dirty = true;
}

bool PostEffectGraph::DependsOn(int nodeId, int targetId) const {
    // nodeId の上流を辿って targetId に到達するか
    if (nodeId == targetId) { return true; }
    const PostEffectNode* node = FindNode(nodeId);
    if (!node) { return false; }
    for (auto& pin : node->inputs) {
        const Link* link = FindLinkToPin(pin.id);
        if (!link) { continue; }
        const Pin* start = FindPin(link->startPinId);
        if (start && DependsOn(start->parentNodeId, targetId)) { return true; }
    }
    return false;
}

bool PostEffectGraph::CanConnect(int startPinId, int endPinId) const {
    const Pin* s = FindPin(startPinId);
    const Pin* e = FindPin(endPinId);
    if (!s || !e) { return false; }
    if (s->pinKind != PinKind::kOutput || e->pinKind != PinKind::kInput) { return false; }
    if (s->parentNodeId == e->parentNodeId) { return false; }
    // start側のノードがすでにend側のノードに依存していたら、繋ぐと循環する
    return !DependsOn(s->parentNodeId, e->parentNodeId);
}

bool PostEffectGraph::TopoSort(int nodeId, std::unordered_set<int>& visited,
    std::unordered_set<int>& visiting, std::vector<int>& order) const {
    if (visited.contains(nodeId)) { return true; }
    if (visiting.contains(nodeId)) { return false; } // 循環
    visiting.insert(nodeId);

    const PostEffectNode* node = FindNode(nodeId);
    if (!node) { return false; }

    for (auto& pin : node->inputs) {
        const Link* link = FindLinkToPin(pin.id);
        if (!link) { continue; }
        const Pin* start = FindPin(link->startPinId);
        if (start && !TopoSort(start->parentNodeId, visited, visiting, order)) { return false; }
    }

    visiting.erase(nodeId);
    visited.insert(nodeId);
    order.push_back(nodeId);
    return true;
}

bool PostEffectGraph::BuildOrder(std::vector<int>& order) const {
    order.clear();
    const PostEffectNode* out = FindOutputNode();
    if (!out) { return false; }
    std::unordered_set<int> visited, visiting;
    return TopoSort(out->id, visited, visiting, order);
}

bool PostEffectGraph::IsPinLinked(int pinId) const {
    for (auto& l : links) { if (l.startPinId == pinId || l.endPinId == pinId) return true; }
    return false;
}

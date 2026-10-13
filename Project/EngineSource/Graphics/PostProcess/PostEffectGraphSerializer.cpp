#include "pch.h"
#include "PostEffectGraphSerializer.h"
#include "LogManager.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace GameEngine;

namespace {
    constexpr const char* kKindScene = "Scene";
    constexpr const char* kKindOutput = "Output";
    constexpr const char* kKindEffect = "Effect";
    constexpr int kGraphVersion = 1;

    // "pos": [x, y] 
    void ReadPosition(const nlohmann::json& n, PostEffectNode& node) {
        if (!n.contains("pos")) { return; }
        const auto& p = n["pos"];
        if (p.is_array() && p.size() == 2 && p[0].is_number() && p[1].is_number()) {
            node.pos.x = p[0].get<float>();
            node.pos.y = p[1].get<float>();
        }
    }
}

bool PostEffectGraphSerializer::Save(const std::string& filePath, const PostEffectGraph& graph, const PostEffectRegistry& registry) {
    // 未登録のエフェクトは保存しない
    auto IsSaved = [&](const PostEffectNode& n) {
        return n.kind != PostEffectNodeKind::kEffect || registry.Owns(n.name, n.effect);
        };

    nlohmann::json root;
    root["version"] = kGraphVersion;
    root["nodes"] = nlohmann::json::array();
    root["links"] = nlohmann::json::array();

    for (const auto& up : graph.nodes) {
        const PostEffectNode& node = *up;
        if (!IsSaved(node)) { continue; }

        nlohmann::json n;
        n["id"] = node.id;
        n["pos"] = nlohmann::json::array({ node.pos.x, node.pos.y });

        switch (node.kind) {
        case PostEffectNodeKind::kScene:
            n["kind"] = kKindScene;
            break;
        case PostEffectNodeKind::kOutput:
            n["kind"] = kKindOutput;
            break;
        case PostEffectNodeKind::kEffect:
            n["kind"] = kKindEffect;
            n["type"] = node.typeName;
            n["name"] = node.name;
            n["active"] = node.effect->IsActive();
            n["passName"] = node.passName;
            n["params"] = nlohmann::json::object();
            break;
        }
        root["nodes"].push_back(std::move(n));
    }

    for (const Link& l : graph.links) {
        const PostEffectNode* from = graph.FindNodeByPin(l.startPinId);
        const PostEffectNode* to = graph.FindNodeByPin(l.endPinId);
        if (!from || !to || !IsSaved(*from) || !IsSaved(*to)) { continue; }

        auto slotIt = std::ranges::find_if(to->inputs, [&](const Pin& p) { return p.id == l.endPinId; });
        if (slotIt == to->inputs.end()) { continue; }
        const int slot = static_cast<int>(slotIt - to->inputs.begin());

        root["links"].push_back({ {"from", from->id}, {"to", to->id}, {"slot", slot} });
    }

    JsonSerializer::SaveToFile(filePath, root);
    return true;
}

bool PostEffectGraphSerializer::Load(const std::string& filePath, const PostEffectRegistry& registry, Loaded& out) {
    if (!JsonSerializer::FileExists(filePath)) { return false; }

    const nlohmann::json root = JsonSerializer::LoadFromFile(filePath);
    if (!root.is_object() || !root.contains("nodes") || !root["nodes"].is_array()) {
        LogManager::GetInstance().Log("PostEffectGraph: jsonの形式が不正です\n");
        return false;
    }

    out = Loaded{};
    Loader loader(registry, out);
    return loader.Run(root);
}

bool Loader::Run(const nlohmann::json& root) {
    LoadNodes(root["nodes"]);
    if (!hasScene_ || !hasOutput_) {
        LogManager::GetInstance().Log("PostEffectGraph: Scene/Outputノードがありません\n");
        return false;
    }
    LoadLinks(root);
    Finish();
    return true;
}

void Loader::LoadNodes(const nlohmann::json& nodes) {
    for (const nlohmann::json& n : nodes) {
        try {
            const int oldId = n.at("id").get<int>();
            PostEffectNode* node = LoadNode(n);
            if (!node) { continue; }

            ReadPosition(n, *node);
            idMap_[oldId] = node;   // 保存時のID → 新しいノード
        }
        catch (const nlohmann::json::exception&) {
            LogManager::GetInstance().Log("PostEffectGraph: ノードの読み込みに失敗しました\n");
        }
    }
}

PostEffectNode* Loader::LoadNode(const nlohmann::json& n) {
    const std::string kind = n.at("kind").get<std::string>();

    // SceneとOutputは1つだけ
    if (kind == kKindScene) {
        if (hasScene_) { return nullptr; }
        hasScene_ = true;
        return out_.graph.AddSceneNode();
    }
    if (kind == kKindOutput) {
        if (hasOutput_) { return nullptr; }
        hasOutput_ = true;
        return out_.graph.AddOutputNode();
    }
    if (kind == kKindEffect) { return LoadEffectNode(n); }
    return nullptr;
}

PostEffectNode* Loader::LoadEffectNode(const nlohmann::json& n) {
    std::string type = n.value("type", std::string());
    if (type.empty()) {
        // version1 互換: "effect": "BloomPass" → "Bloom"
        type = n.at("effect").get<std::string>();
        if (type.ends_with("Pass")) { type.resize(type.size() - 4); }
    }

    auto fx = registry_.CreateInstance(type);
    if (!fx) {
        LogManager::GetInstance().Log("PostEffectGraph: 未登録の種類をスキップ: " + type + "\n");
        return nullptr;
    }

    const std::string name = MakeUniqueName(n.value("name", type), out_.effects);
    const std::string passName = ResolvePassName(n);

    IPostEffect* raw = fx.get();
    out_.effects[name] = std::move(fx);
    pending_.push_back({ raw, n.value("active", raw->IsActive()),
        n.contains("params") ? n["params"] : nlohmann::json::object() });

    return out_.graph.AddEffectNode(type, name, passName, raw);
}

std::string Loader::ResolvePassName(const nlohmann::json& n) {
    std::string passName = n.value("passName", n.value("persistentPass", std::string()));
    if (passName.empty()) { return {}; }

    if (!usedPasses_.insert(passName).second) { return {}; }

    out_.passesToCreate.push_back(passName);
    return passName;
}

void Loader::LoadLinks(const nlohmann::json& root) {
    if (!root.contains("links") || !root["links"].is_array()) { return; }

    int total = 0, loaded = 0;
    for (const nlohmann::json& l : root["links"]) {
        ++total;
        try {
            const int fromId = l.at("from").get<int>();
            const int toId = l.at("to").get<int>();
            const int slot = l.at("slot").get<int>();

            auto s = idMap_.find(fromId);
            auto e = idMap_.find(toId);
            if (s == idMap_.end() || e == idMap_.end()) {
                LogManager::GetInstance().Log("PostEffectGraph: リンクのノードが見つかりません from=" + std::to_string(fromId) + " to=" + std::to_string(toId) + "\n");
                continue;
            }

            const PostEffectNode* from = s->second;
            const PostEffectNode* to = e->second;
            if (from->kind == PostEffectNodeKind::kOutput || slot < 0 || slot >= static_cast<int>(to->inputs.size())) {
                LogManager::GetInstance().Log("PostEffectGraph: リンクのスロットが不正です\n");
                continue;
            }
            if (!out_.graph.CanConnect(from->output.id, to->inputs[slot].id)) {
                LogManager::GetInstance().Log("PostEffectGraph: CanConnectが拒否しました " + from->name + " -> " + to->name + "\n");
                continue;
            }
            out_.graph.AddLink(from->output.id, to->inputs[slot].id);
            ++loaded;
        }
        catch (const nlohmann::json::exception&) {
            LogManager::GetInstance().Log("PostEffectGraph: リンクの読み込みに失敗しました\n");
        }
    }
    LogManager::GetInstance().Log("PostEffectGraph: リンク " + std::to_string(loaded) + "/" + std::to_string(total) + " 本を読み込みました\n");
}

// 成功が確定したので、エフェクトの状態を反映する
void Loader::Finish() {
    for (auto& p : pending_) {
        p.fx->SetIsActive(p.active);
        // TODO: try { p.fx->LoadParams(p.params); } catch (...) {}
    }
}
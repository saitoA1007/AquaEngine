#include "pch.h"
#include "PostEffectManager.h"
#include "LogManager.h"
using namespace GameEngine;

namespace {
    // 重複しない名前を作るヘルプ関数
    template <class Map>
    std::string MakeUniqueName(const std::string& base, const Map& existing) {
        if (!existing.contains(base)) { return base; }
        for (int i = 1;; ++i) {
            std::string s = base + "_" + std::to_string(i);
            if (!existing.contains(s)) { return s; }
        }
    }
}

void PostEffectManager::Initialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager, PSOManager* psoManager, RenderPassController* renderPassController) {
    commandList_ = commandList;
    srvManager_ = srvManager;
    renderPassController_ = renderPassController;

    // 使い回し用のパスを生成
    for (auto& name : poolPassNames_) {
        renderPassController_->AddPass(name);
    }
    // 輝度マスクの描画パスを作成
    EnsurePass("HighLumMaskPass");

    // 生成出来るポストエフェクトのタイプを追加する
    RegisterEffectType<ColorGrading>("ColorGrading", psoManager);
    RegisterEffectType<HighLumMask>("HighLumMask", psoManager);
    RegisterEffectType<GaussVertical>("GaussVertical", psoManager);
    RegisterEffectType<GaussHorizontal>("GaussHorizontal", psoManager);
    RegisterEffectType<Bloom>("Bloom", psoManager);
    RegisterEffectType<Dissolve>("Dissolve", psoManager);

    // デフォルトのノードが繋がっている状態
    BuildDefaultGraph();
    // 保存されているノードを呼び出す
    LoadGraph(kGraphFilePath);
}

void PostEffectManager::Execute() {
    // 削除済みエフェクトの遅延破棄
    for (auto& r : retired_) { --r.framesLeft; }
    std::erase_if(retired_, [](const Retired& r) { return r.framesLeft < 0; });


    if (graph_.dirty) { RebuildOrder(); }

    const std::string scenePass = renderPassController_->GetSceneFinalPass();

    constexpr int kSceneRes = 0;
    std::vector<Resource> resources;
    resources.push_back({ renderPassController_->GetSrvIndex(scenePass), -1, 0, scenePass });

    // 出力ピンID,resources添字
    std::unordered_map<int, int> pinToRes;
    std::array<bool, kPoolSize> slotBusy{};

    // 今回実行されるノードだけを読み手として数える
    const std::unordered_set<int> active(executeOrder_.begin(), executeOrder_.end());
    auto consumers = [&](int outPinId) {
        int n = 0;
        for (const Link& l : graph_.links) {
            if (l.startPinId != outPinId) { continue; }
            const PostEffectNode* end = graph_.FindNodeByPin(l.endPinId);
            if (end && active.contains(end->id)) { ++n; }
        }
        return n;
        };
    auto release = [&](int idx) {
        Resource& r = resources[idx];
        if (--r.refCount == 0 && r.poolSlot >= 0) { slotBusy[r.poolSlot] = false; }
        };

    // 順序構築に失敗したらシーンを出力する
    std::string finalPass = scenePass;   

    for (int nodeId : executeOrder_) {
        PostEffectNode* node = graph_.FindNode(nodeId);
        if (!node) { continue; }

        if (node->kind == PostEffectNodeKind::kScene) {
            pinToRes[node->output.id] = kSceneRes;
            continue;
        }

        // 入力
        std::vector<int> inRes;
        for (const Pin& pin : node->inputs) {
            const Link* link = graph_.FindLinkToPin(pin.id);
            auto it = link ? pinToRes.find(link->startPinId) : pinToRes.end();
            inRes.push_back(it != pinToRes.end() ? it->second : kSceneRes);
        }

        if (node->kind == PostEffectNodeKind::kOutput) {
            finalPass = resources[inRes[0]].passName;
            continue;
        }

        IPostEffect* fx = node->effect;
        for (uint32_t s = 0; s < inRes.size(); ++s) {
            fx->SetInputIndex(s, resources[inRes[s]].srvIndex);
        }

        // 無効の場合は指定スロットの入力をそのまま抜ける
        if (!fx->IsActive()) {
            const uint32_t pt = fx->GetPassThroughSlot();
            for (uint32_t s = 0; s < inRes.size(); ++s) {
                if (s != pt) { release(inRes[s]); }
            }
            resources[inRes[pt]].refCount += consumers(node->output.id) - 1;
            pinToRes[node->output.id] = inRes[pt];
            continue;
        }

        // 出力先の決定
        std::string outPass;
        int slot = -1;
        if (!node->passName.empty()) {
            outPass = node->passName;
        } else {
            for (int i = 0; i < (int)kPoolSize; ++i) { if (!slotBusy[i]) { slot = i; break; } }
            if (slot < 0) {
                LogManager::GetInstance().Log("PostEffect: プールのRTが不足しています\n");
                assert(false);
                continue;
            }
            slotBusy[slot] = true;
            outPass = poolPassNames_[slot];
        }
        resources.push_back({ renderPassController_->GetSrvIndex(outPass), slot,
                              consumers(node->output.id), outPass });
        pinToRes[node->output.id] = (int)resources.size() - 1;

        // 描画
        fx->Update();
        renderPassController_->PrePass(outPass);
        renderPassController_->ClearRenderPass(outPass);
        PreDraw(fx->GetPsoName());
        fx->Draw(commandList_, srvManager_);
        renderPassController_->PostPass(outPass);

        // 入力の解放
        for (int idx : inRes) { release(idx); }
    }

    currentPassName_ = finalPass;
    renderPassController_->SetPostProcessFinalPass(currentPassName_);
    renderPassController_->SetPresentPass(currentPassName_);
}

void PostEffectManager::PreDraw(const std::string& psoName) {
    auto it = psoList_.find(psoName);
    assert(it != psoList_.end() && "未登録のPSO名です");

    commandList_->SetGraphicsRootSignature(it->second.rootSignature);
    commandList_->SetPipelineState(it->second.graphicsPipelineState);
}

void PostEffectManager::BuildDefaultGraph() {
    graph_ = PostEffectGraph{};  

    auto* scene = graph_.AddSceneNode();
    auto* output = graph_.AddOutputNode();

    auto* highLum = CreateEffectNode("HighLumMask");
    SetPersistent(highLum->id, true);  // 輝度マスクは保存パスに描く
    auto* gaussV = CreateEffectNode("GaussVertical");
    auto* gaussH = CreateEffectNode("GaussHorizontal");
    auto* bloom = CreateEffectNode("Bloom");
    auto* grading = CreateEffectNode("ColorGrading");
    auto* dissolve = CreateEffectNode("Dissolve");
    
    // Scene → HighLum → GaussV → GaussH → Bloom(Blur) → Grading → Dissolve → Output
    graph_.AddLink(scene->output.id, highLum->inputs[0].id);
    graph_.AddLink(highLum->output.id, gaussV->inputs[0].id);
    graph_.AddLink(gaussV->output.id, gaussH->inputs[0].id);
    graph_.AddLink(gaussH->output.id, bloom->inputs[0].id);   // Blur
    graph_.AddLink(scene->output.id, bloom->inputs[1].id);   // 元の画像
    graph_.AddLink(bloom->output.id, grading->inputs[0].id);
    graph_.AddLink(grading->output.id, dissolve->inputs[0].id);
    graph_.AddLink(dissolve->output.id, output->inputs[0].id);
}

void PostEffectManager::RebuildOrder() {
    std::vector<int> order;
    if (graph_.BuildOrder(order)) {
        executeOrder_ = std::move(order);
    } else {
        // 循環など。前回の有効な順序を使い続ける
        LogManager::GetInstance().Log("PostEffectGraph: 実行順の構築に失敗しました\n");
    }
    graph_.dirty = false;
}

bool PostEffectManager::SaveGraph(const std::string& filePath) {
    // IPostEffect,effects_ のキー
    std::unordered_map<const IPostEffect*, std::string> keyOf;
    for (auto& [key, fx] : effects_) { keyOf[fx.get()] = key; }

    nlohmann::json root;
    root["version"] = 1;
    root["nodes"] = nlohmann::json::array();
    root["links"] = nlohmann::json::array();

    for (auto& up : graph_.nodes) {
        const PostEffectNode* node = up.get();
        nlohmann::json n;
        n["id"] = node->id;
        n["pos"] = nlohmann::json::array({ node->pos.x, node->pos.y });

        switch (node->kind) {
        case PostEffectNodeKind::kScene:
            n["kind"] = "Scene";
            break;
        case PostEffectNodeKind::kOutput:
            n["kind"] = "Output";
            break;
        case PostEffectNodeKind::kEffect: {
            auto it = keyOf.find(node->effect);
            // 未登録のエフェクトは保存しない
            if (it == keyOf.end()) { continue; }
            n["kind"] = "Effect";
            n["type"] = node->typeName;
            n["name"] = node->name;
            n["active"] = node->effect->IsActive();
            n["passName"] = node->passName;
            nlohmann::json params = nlohmann::json::object();
            //node->effect->SaveParams(params);
            n["params"] = std::move(params);
            break;
        }
        }
        root["nodes"].push_back(std::move(n));
    }

    for (const Link& l : graph_.links) {
        const PostEffectNode* from = graph_.FindNodeByPin(l.startPinId);
        const PostEffectNode* to = graph_.FindNodeByPin(l.endPinId);
        if (!from || !to) { continue; }
        int slot = -1;
        for (size_t i = 0; i < to->inputs.size(); ++i) {
            if (to->inputs[i].id == l.endPinId) { slot = (int)i; break; }
        }
        if (slot < 0) { continue; }
        root["links"].push_back({ {"from", from->id}, {"to", to->id}, {"slot", slot} });
    }

    JsonSerializer::SaveToFile(filePath, root);
    return true;
}

bool PostEffectManager::LoadGraph(const std::string& filePath) {
    if (!JsonSerializer::FileExists(filePath)) { return false; }

    const nlohmann::json root = JsonSerializer::LoadFromFile(filePath);
    if (!root.is_object() || !root.contains("nodes") || !root["nodes"].is_array()) {
        LogManager::GetInstance().Log("PostEffectGraph: jsonの形式が不正です\n");
        return false;
    }

    PostEffectGraph tmp;
    std::unordered_map<int, PostEffectNode*> idMap;
    // 今回のロードで作るエフェクト(失敗したらここで破棄される)
    std::unordered_map<std::string, std::unique_ptr<IPostEffect>> newEffects;
    std::unordered_set<std::string> usedPasses;
    std::vector<std::string> passesToCreate;
    bool hasScene = false, hasOutput = false;

    struct Pending { IPostEffect* fx; bool active; nlohmann::json params; };
    std::vector<Pending> pending;

    for (const nlohmann::json& n : root["nodes"]) {
        try {
            const int oldId = n.at("id").get<int>();
            const std::string kind = n.at("kind").get<std::string>();
            PostEffectNode* node = nullptr;

            if (kind == "Scene") {
                if (hasScene) { continue; }
                node = tmp.AddSceneNode();
                hasScene = true;
            } else if (kind == "Output") {
                if (hasOutput) { continue; }
                node = tmp.AddOutputNode();
                hasOutput = true;
            } else if (kind == "Effect") {
                std::string type = n.value("type", std::string());
                if (type.empty()) {
                    // version1 互換: "effect": "BloomPass" → "Bloom"
                    type = n.at("effect").get<std::string>();
                    if (type.ends_with("Pass")) { type.resize(type.size() - 4); }
                }
                auto f = factories_.find(type);
                if (f == factories_.end()) {
                    LogManager::GetInstance().Log("PostEffectGraph: 未登録の種類をスキップ: " + type + "\n");
                    continue;
                }

                const std::string name = MakeUniqueName(n.value("name", type), newEffects);

                // 保存パス名: 空 = プール。プール名や他ノードと重複するものは無効にする
                std::string passName = n.value("passName", n.value("persistentPass", std::string()));
                const bool reserved = std::find(poolPassNames_.begin(), poolPassNames_.end(), passName)
                    != poolPassNames_.end();
                if (reserved || (!passName.empty() && !usedPasses.insert(passName).second)) {
                    passName.clear();
                } else if (!passName.empty()) {
                    passesToCreate.push_back(passName);
                }

                auto fx = f->second();
                IPostEffect* raw = fx.get();
                newEffects[name] = std::move(fx);
                node = tmp.AddEffectNode(type, name, passName, raw);
                pending.push_back({ raw, n.value("active", raw->IsActive()),
                    n.contains("params") ? n["params"] : nlohmann::json::object() });
            } else {
                continue;
            }

            if (n.contains("pos") && n["pos"].is_array() && n["pos"].size() == 2
                && n["pos"][0].is_number() && n["pos"][1].is_number()) {
                node->pos.x = n["pos"][0].get<float>();
                node->pos.y = n["pos"][1].get<float>();
            }
            // 保存時のID → 新しいノード
            idMap[oldId] = node;

            // ノードのリンクを繋ぐ
            if (root.contains("links") && root["links"].is_array()) {
                int total = 0, loaded = 0;
                for (const nlohmann::json& l : root["links"]) {
                    ++total;
                    try {
                        const int fromId = l.at("from").get<int>();
                        const int toId = l.at("to").get<int>();
                        const int slot = l.at("slot").get<int>();

                        auto s = idMap.find(fromId);
                        auto e = idMap.find(toId);
                        if (s == idMap.end() || e == idMap.end()) {
                            LogManager::GetInstance().Log("PostEffectGraph: リンクのノードが見つかりません from=" + std::to_string(fromId) + " to=" + std::to_string(toId) + "\n");
                            continue;
                        }

                        PostEffectNode* from = s->second;
                        PostEffectNode* to = e->second;
                        if (from->kind == PostEffectNodeKind::kOutput || slot < 0 || slot >= (int)to->inputs.size()) {
                            LogManager::GetInstance().Log("PostEffectGraph: リンクのスロットが不正です\n");
                            continue;
                        }
                        if (!tmp.CanConnect(from->output.id, to->inputs[slot].id)) {
                            LogManager::GetInstance().Log("PostEffectGraph: CanConnectが拒否しました "  + from->name + " -> " + to->name + "\n");
                            continue;
                        }
                        tmp.AddLink(from->output.id, to->inputs[slot].id);
                        ++loaded;
                    }
                    catch (const nlohmann::json::exception&) {
                        LogManager::GetInstance().Log("PostEffectGraph: リンクの読み込みに失敗しました\n");
                    }
                }
                LogManager::GetInstance().Log("PostEffectGraph: リンク " + std::to_string(loaded) + "/" + std::to_string(total) + " 本を読み込みました\n");
            }
        }
        catch (const nlohmann::json::exception&) {
            LogManager::GetInstance().Log("PostEffectGraph: ノードの読み込みに失敗しました\n");
        }
    }

    if (!hasScene || !hasOutput) {
        LogManager::GetInstance().Log("PostEffectGraph: Scene/Outputノードがありません\n");
        return false;
    }

    // ここまで成功したら反映する
    for (auto& p : pending) {
        p.fx->SetIsActive(p.active);
        //try { p.fx->LoadParams(p.params); }
    }
    for (auto& pass : passesToCreate) { EnsurePass(pass); }

    RetireAllEffects();
    effects_ = std::move(newEffects);
    graph_ = std::move(tmp);
    executeOrder_.clear();
    return true;
}

PostEffectNode* PostEffectManager::CreateEffectNode(const std::string& typeName, const std::string& name) {
    auto f = factories_.find(typeName);
    if (f == factories_.end()) {
        LogManager::GetInstance().Log("PostEffect: 未登録の種類です: " + typeName + "\n");
        return nullptr;
    }
    const std::string instName = MakeUniqueName(name.empty() ? typeName : name, effects_);
    auto fx = f->second();
    IPostEffect* raw = fx.get();
    effects_[instName] = std::move(fx);
    return graph_.AddEffectNode(typeName, instName, "", raw);
}

void PostEffectManager::DestroyEffectNode(int nodeId) {
    PostEffectNode* node = graph_.FindNode(nodeId);
    if (!node || node->kind != PostEffectNodeKind::kEffect) { return; }
    const std::string name = node->name;

    graph_.RemoveNode(nodeId);   // これで node は無効になる

    auto it = effects_.find(name);
    if (it != effects_.end()) {
        retired_.push_back({ std::move(it->second), kRetireFrames });
        effects_.erase(it);
    }
}

void PostEffectManager::SetPersistent(int nodeId, bool enable) {
    PostEffectNode* node = graph_.FindNode(nodeId);
    if (!node || node->kind != PostEffectNodeKind::kEffect) { return; }
    if (enable) {
        node->passName = node->name + "Pass";   // インスタンス名ごとに専用パス
        EnsurePass(node->passName);
    } else {
        node->passName.clear();                 // プールに戻す
    }
}

void PostEffectManager::RetireAllEffects() {
    for (auto& [key, fx] : effects_) { retired_.push_back({ std::move(fx), kRetireFrames }); }
    effects_.clear();
}

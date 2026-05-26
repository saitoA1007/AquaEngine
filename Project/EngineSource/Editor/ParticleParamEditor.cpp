#include "ParticleParamEditor.h"
#include "ImGuiManager.h"
using json = nlohmann::json;
using namespace GameEngine;

void ParticleParamEditor::RemoveGroup(const std::string& path) {
    auto segs = SplitPath(path);
    if (segs.empty()) { root_.children.clear(); root_.entries.clear(); return; }

    Group* cur = &root_;
    for (size_t i = 0; i < segs.size() - 1; ++i) {
        auto it = cur->children.find(segs[i]);
        if (it == cur->children.end()) { return; }
        cur = &it->second;
    }
    cur->children.erase(segs.back());
}

void ParticleParamEditor::RemoveItem(const std::string& path, const std::string& label) {
    // 指定されたパスのグループを探す
    Group* targetGroup = FindGroup(path);
    if (!targetGroup) { return; }

    // ラベルが一致する要素を削除
    std::erase_if(targetGroup->entries, [&label](const std::unique_ptr<IParamEntry>& entry) {
        return entry->label == label;
        });
}

void ParticleParamEditor::DrawImgui() {
    DrawGroup(root_);
}

void ParticleParamEditor::DrawGroup(Group& group) {
    DrawEntries(group);
    for (auto& [childName, childGroup] : group.children) {
        ImGui::PushID(childName.c_str());
        if (ImGui::TreeNode(childName.c_str())) {
            DrawGroup(childGroup);
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void ParticleParamEditor::DrawEntries(Group& group) {
    // priority 順にソートしてから描画
    std::vector<IParamEntry*> sorted;
    sorted.reserve(group.entries.size());
    for (auto& e : group.entries) { sorted.push_back(e.get()); }
    std::sort(sorted.begin(), sorted.end(),
        [](const IParamEntry* a, const IParamEntry* b) {
            return a->priority != b->priority
                ? a->priority < b->priority
                : a->label < b->label;
        });

    for (auto* e : sorted) {
        ImGui::PushID(e->label.c_str());
        e->DrawImGui();
        ImGui::Separator();
        ImGui::PopID();
    }
}

ParticleParamEditor::Group& ParticleParamEditor::ResolveGroup(const std::string& path) {
    if (path.empty()) { return root_; }
    auto segs = SplitPath(path);
    Group* cur = &root_;
    for (const auto& s : segs) {
        cur = &cur->children[s];
    }
    return *cur;
}

ParticleParamEditor::Group* ParticleParamEditor::FindGroup(const std::string& path) {
    if (path.empty()) { return &root_; }
    auto segs = SplitPath(path);
    Group* cur = &root_;
    for (const auto& s : segs) {
        auto it = cur->children.find(s);
        if (it == cur->children.end()) { return nullptr; }
        cur = &it->second;
    }
    return cur;
}

void ParticleParamEditor::Serialize(json& root) const {
    SerializeGroup(root, root_);
}

void ParticleParamEditor::Deserialize(const json& root) {
    DeserializeGroup(root, root_);
}

void ParticleParamEditor::SerializeGroup(json& node, const Group& group) const {
    for (const auto& e : group.entries) {
        json child;
        e->Serialize(child);
        node[e->label] = child;
    }
    for (const auto& [childName, childGroup] : group.children) {
        node[childName] = json::object();
        SerializeGroup(node[childName], childGroup);
    }
}

void ParticleParamEditor::DeserializeGroup(const json& node, Group& group) {
    if (!node.is_object()) { return; }
    for (auto& e : group.entries) {
        if (node.contains(e->label)) {
            e->Deserialize(node[e->label]);
        }
    }
    for (auto& [childName, childGroup] : group.children) {
        if (node.contains(childName)) {
            DeserializeGroup(node[childName], childGroup);
        }
    }
}

std::vector<std::string> ParticleParamEditor::SplitPath(const std::string& path) {
    std::vector<std::string> segs;
    std::stringstream ss(path);
    std::string seg;
    while (std::getline(ss, seg, '/')) {
        if (!seg.empty()) segs.push_back(seg);
    }
    return segs;
}

#include "InspectorWindow.h"

using namespace GameEngine;

void InspectorWindow::Draw() {
    if (!isActive) return;

    if (!ImGui::Begin("ParameterInspector", &isActive)) {
        ImGui::End();
        return;
    }

    const std::string& selectedPath = GameParamEditor::GetInstance()->GetRootGroupName();

    // グループが選択されていない場合の表示
    if (selectedPath.empty()) {
        ImGui::TextDisabled("No group selected");
        ImGui::End();
        return;
    }

    // パスからルートグループ名を取得
    const std::string rootGroupName = selectedPath.substr(0, selectedPath.find('/'));

    // ルートグループが存在するかチェック
    auto& allGroups = GameParamEditor::GetInstance()->GetAllGroups();
    auto itRoot = allGroups.find(rootGroupName);
    if (itRoot == allGroups.end()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Group not found: %s", rootGroupName.c_str());
        GameParamEditor::GetInstance()->SetRootGroupName("");
        ImGui::End();
        return;
    }

    // 選択したパスをヘッダーに表示
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Group: %s", selectedPath.c_str());
    ImGui::Separator();

    // 保存ボタン
    if (ImGui::Button("Save")) {
        GameParamEditor::GetInstance()->SaveFile(rootGroupName);
        std::string message = std::format("{}.json saved.", rootGroupName);
        MessageBoxA(nullptr, message.c_str(), "GameParamEditor", 0);
    }

    ImGui::SameLine();

    // 読み込みボタン
    if (ImGui::Button("Load")) {
        GameParamEditor::GetInstance()->LoadFile(rootGroupName);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ルートグループ全体を再帰的に描画
    DrawGroup(itRoot->second);

    ImGui::End();
}

void InspectorWindow::DrawGroup(GameParamEditor::Group& group) {

    // このグループのアイテムを描画
    DrawItems(group);

    // サブグループを再帰的に
    for (auto& [childName, childGroup] : group.children) {
        ImGui::PushID(childName.c_str());

        if (ImGui::TreeNode(childName.c_str())) {
            // 再帰
            DrawGroup(childGroup);
            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

void InspectorWindow::DrawItems(GameParamEditor::Group& group) {
    if (group.items.empty()) { return; }

    // 優先順位でソート
    std::vector<std::pair<std::string, GameParamEditor::Item*>> sortedItems;
    for (auto& [itemName, item] : group.items) {
        sortedItems.push_back({ itemName, &item });
    }
    std::sort(sortedItems.begin(), sortedItems.end(),
        [](const auto& a, const auto& b) {
            if (a.second->priority != b.second->priority) {
                return a.second->priority < b.second->priority;
            }
            return a.first < b.first;
        }
    );

    // ソート済みの順序で描画
    for (auto& [itemName, itemPtr] : sortedItems) {
        ImGui::PushID(itemName.c_str());
        std::visit(DebugParameterVisitor{ itemName }, itemPtr->value);
        ImGui::PopID();
    }
}
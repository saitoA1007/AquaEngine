#include "HierarchyWindow.h"
#include "GameParamEditor.h"
#include "ImGuiManager.h"
#include <format>

using namespace GameEngine;

void HierarchyWindow::Draw() {
    if (!isActive) return;

    if (!ImGui::Begin("ParameterHierarchy", &isActive)) {
        ImGui::End();
        return;
    }

    const std::string& activeSceneName = GameParamEditor::GetInstance()->GetActiveScene();

    // シーンフィルタ表示
    if (activeSceneName.empty() || activeSceneName == "None") {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "All Groups");
        ImGui::Separator();
    } else {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Active Scene: %s", activeSceneName.c_str());
        ImGui::Separator();
    }

    const std::string& selectedGroupName = GameParamEditor::GetInstance()->GetSelectGroup();

    // 各グループをリスト表示
    for (auto& [groupName, group] : GameParamEditor::GetInstance()->GetAllGroups()) {

        // 指定したシーンにあった項目を表示する
        if (!activeSceneName.empty() && activeSceneName != "None" &&
            !group.sceneName.empty() &&
            group.sceneName != activeSceneName) {
            continue;
        }

        bool isSelected = (selectedGroupName == groupName);

        // 項目数を表示
        std::string label = groupName;
        if (ImGui::Selectable(label.c_str(), isSelected)) {
            GameParamEditor::GetInstance()->SelectGroup(groupName);
        }

        // 右クリックメニュー
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Save")) {
                GameParamEditor::GetInstance()->SaveFile(groupName);
                std::string message = std::format("{}.json saved.", groupName);
                MessageBoxA(nullptr, message.c_str(), "GameParamEditor", 0);
            }
            if (ImGui::MenuItem("Load")) {
                GameParamEditor::GetInstance()->LoadFile(groupName);
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}
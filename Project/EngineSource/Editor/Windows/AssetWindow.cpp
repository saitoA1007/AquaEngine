#include"AssetWindow.h"
#include"ImGuiManager.h"

using namespace GameEngine;

void AssetWindow::Draw() {
    ImGui::Begin("Asset", &isActive);
    ImGui::Text("None");
    ImGui::End();
}
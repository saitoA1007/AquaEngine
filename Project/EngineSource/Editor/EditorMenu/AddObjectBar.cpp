#include "AddObjectBar.h"
#include "ImGuiManager.h"
using namespace GameEngine;

AddObjectBar::AddObjectBar(StaticGameObjectManager* staticObjectManager, RenderQueue* renderQueue) {
	staticObjectManager_ = staticObjectManager;
	renderQueue_ = renderQueue;
}

void AddObjectBar::Run() {

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Object")) {
			if (ImGui::MenuItem("Cube")) {
				staticObjectManager_->AddObject("CubeObject", "Cube");
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGuiIO& io = ImGui::GetIO();

	// マウスがクリックされた時
	if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		if (!io.WantCaptureMouse) {
			// スクリーン座標のマウス位置を取得
			Vector2 mousePos = Vector2(io.MousePos.x, io.MousePos.y);
			Camera& camera = renderQueue_->GetMainCamera();
			int32_t id = staticObjectManager_->SelectObject(mousePos, camera.GetViewMatrix(), camera.GetProjectionMatrix(), camera.GetWorldPosition());
			if (id <= -1) {
				selectObject_ = nullptr;
			} else {
				// 選択しているオブジェクトを取得
				selectObject_ = staticObjectManager_->GetStaticObject(id);
			}			
		}
	}
}
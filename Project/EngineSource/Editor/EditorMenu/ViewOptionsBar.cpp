#include "ViewOptionsBar.h"
#include "ImGuiManager.h"
#include "RenderQueue.h"
#include "DebugCamera.h"
#include "DebugRenderer.h"
using namespace GameEngine;

ViewOptionsBar::ViewOptionsBar(Input* input, RenderQueue* renderQueue, DebugRenderer* debugRenderer, Model* gridModel) {
	// デバック用グリッドのワールド行列を初期化
	gridWorldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// デバックカメラを生成
	debugCamera_ = std::make_unique<DebugCamera>(input);
	debugCamera_->Initialize({ 0.0f,2.0f,-20.0f }, 1280, 720);
	debugCamera_->Update();

	// モデルを取得
	gridModel_ = gridModel;
	// 描画機能
	renderQueue_ = renderQueue;
	// デバック描画機能
	debugRenderer_ = debugRenderer;
}

void ViewOptionsBar::Run() {

	// メインメニュー
	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("View")) {
			ImGui::Checkbox("isDebugView", &isDebugView_);
			ImGui::Checkbox("isDebugDraw", &isDebugDraw_);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// デバック
	if (isDebugView_) {
		// デバックカメラを操作
		debugCamera_->Update();
		// デバックカメラを設定
		renderQueue_->SetDebugCamera(debugCamera_->GetConstantBuffer());

		// グリッドの更新処理
		gridWorldTransform_.transform_.translate = Vector3(debugCamera_->GetTargetPosition().x, -0.1f, debugCamera_->GetTargetPosition().z);
		gridWorldTransform_.UpdateTransformMatrix();
	}

	// デバック描画
	if (isDebugDraw_) {
		// グリッドを描画
		renderQueue_->SubmitGrid(gridModel_, gridWorldTransform_);
	}

	// デバック描画の表示設定
	debugRenderer_->SetEnabled(isDebugDraw_);
}
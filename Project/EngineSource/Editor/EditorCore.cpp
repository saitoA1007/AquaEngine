#include "EditorCore.h"
#include "ImGuiManager.h"
#include "SceneChangeRequest.h"

// デバック機能
#include "EditorMenu/EditorWindowManager.h"
#include "EditorMenu/EditorMenuBar.h"
#include "EditorMenu/EditorLayout.h"
#include "EditorMenu/EditorToolBar.h"
#include "EditorMenu/SceneMenuBar.h"

// デバックウィンドウ
#ifdef USE_IMGUI
#include "Windows/SceneWIndow.h"
#include "Windows/AssetWindow.h"
#include "Windows/ConsoleWindow.h"
#include "Windows/HierarchyWindow.h"
#include "Windows/InspectorWindow.h"
#include "Windows/PerformanceWindow.h"
#endif

using namespace GameEngine;

EditorCore::EditorCore() {}
EditorCore::~EditorCore() {}

void EditorCore::Initialize(TextureManager* textureManager, SceneChangeRequest* sceneChangeRequest, RenderPassController* renderPassController) {
	windowManager_ = std::make_unique<EditorWindowManager>();
	menuBar_ = std::make_unique<EditorMenuBar>();
	editorLayout_ = std::make_unique<EditorLayout>();
	editorToolBar_ = std::make_unique<EditorToolBar>(textureManager);
	sceneMenuBar_ = std::make_unique<SceneMenuBar>(sceneChangeRequest);

	// ウィンドウの内容を登録する
	windowManager_->RegisterWindow(std::make_unique<SceneWindow>(renderPassController));
	windowManager_->RegisterWindow(std::make_unique<AssetWindow>());
	windowManager_->RegisterWindow(std::make_unique<HierarchyWindow>());
	windowManager_->RegisterWindow(std::make_unique<InspectorWindow>());
	windowManager_->RegisterWindow(std::make_unique<ConsoleWindow>());
	windowManager_->RegisterWindow(std::make_unique<PerformanceWindow>());

	// レイアウトのデータを取得する
	editorLayout_->LoadLayout(windowManager_->GetWindows());
}

void EditorCore::Run() {

	BeginDockSpace();

	sceneMenuBar_->Run();
	menuBar_->Run(windowManager_.get());
	editorToolBar_->Run();
	windowManager_->DrawAllWindows();
}

void EditorCore::BeginDockSpace() {
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::Begin("DockSpace Window", nullptr, window_flags);
	ImGui::PopStyleVar(2);
	ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
	ImGui::End();
}

void EditorCore::Finalize() {
	// レイアウトデータを保存する
	editorLayout_->SaveLayout(windowManager_->GetWindows());
}

bool EditorCore::IsActiveUpdate() const {
	return editorToolBar_->GetIsActiveUpdate();
}

bool EditorCore::IsPause() const {
	return editorToolBar_->GetIsPauce();
}
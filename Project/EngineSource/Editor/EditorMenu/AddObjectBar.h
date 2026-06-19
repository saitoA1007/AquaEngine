#pragma once
#include "StaticGameObjectManager.h"
#include "ImGuiManager.h"
#include "Command/EditorCommandHistory.h"

namespace GameEngine {

	class DebugCamera;

	class AddObjectBar {
	public:
		AddObjectBar(StaticGameObjectManager* staticObjectManager, RenderQueue* renderQueue, DebugCamera* debugCamera);

		void Run();

		void ApplyGuizmo();

		void AddObjectFromPath(const std::string& filePath);

	private:
		// 配置オブジェクト管理
		StaticGameObjectManager* staticObjectManager_ = nullptr;
		RenderQueue* renderQueue_ = nullptr;
		DebugCamera* debugCamera_ = nullptr;

		// 選択中のオブジェクト
		StaticGameObject* selectObject_ = nullptr;
		int32_t selectedId_ = -1;

		// 現在の選択状態
		ImGuizmo::OPERATION currentOperation_ = ImGuizmo::TRANSLATE;

		// Undo,Redo履歴
		EditorCommandHistory commandHistory_;

		// ギズモのドラッグ開始、終了を検知するための状態
		bool wasUsingGizmo_ = false;
		Transform transformBeforeGizmo_;

		int addedObjectCount_ = 0;

	private:

		// 現在の選択状態をクリアする
		void ClearSelection();
	};
}

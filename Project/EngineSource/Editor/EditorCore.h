#pragma once
#include"TextureManager.h"

#include"EditorMenu/EditorWindowManager.h"
#include"EditorMenu/EditorMenuBar.h"
#include"EditorMenu/EditorLayout.h"
#include"EditorMenu/EditorToolBar.h"
#include"EditorMenu/SceneMenuBar.h"

#ifdef USE_IMGUI
#include"Windows/SceneWIndow.h"
#include"Windows/AssetWindow.h"
#include"Windows/ConsoleWindow.h"
#include"Windows/HierarchyWindow.h"
#include"Windows/InspectorWindow.h"
#include"Windows/PerformanceWindow.h"
#endif

namespace GameEngine {

	class EditorCore {
	public:

		void Initialize(TextureManager* textureManager, SceneChangeRequest* sceneChangeRequest, RenderPassController* renderPassController);

		void Run();

		// 更新状態を取得する
		bool IsActiveUpdate() const;

		bool IsPause() const;

		void Finalize();

	private:

		// 各ウィンドウ
		std::unique_ptr<EditorWindowManager> windowManager_;

		// メニューバー
		std::unique_ptr<EditorMenuBar> menuBar_;

		// シーンの管理機能
		std::unique_ptr<SceneMenuBar> sceneMenuBar_;

		// エディターの表示管理
		std::unique_ptr<EditorLayout> editorLayout_;

		// シーンの操作などをおこなう
		std::unique_ptr<EditorToolBar> editorToolBar_;

	private:

		/// <summary>
		/// Dockをするためのスペースを作成する
		/// </summary>
		void BeginDockSpace();

	};
}

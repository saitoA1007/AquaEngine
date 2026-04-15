#pragma once
#include "TextureManager.h"

class SceneChangeRequest;

namespace GameEngine {

	// 前方宣言
	class EditorWindowManager;
	class EditorMenuBar;
	class SceneMenuBar;
	class EditorLayout;
	class EditorToolBar;

	class RenderPassController;

	class EditorCore {
	public:
		EditorCore();
		~EditorCore();

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

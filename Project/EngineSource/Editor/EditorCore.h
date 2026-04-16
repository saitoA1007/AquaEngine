#pragma once
#include "TextureManager.h"

class SceneChangeRequest;

namespace GameEngine {

	/// 前方宣言
	// エディター機能
	class EditorWindowManager;
	class EditorMenuBar;
	class SceneMenuBar;
	class EditorLayout;
	class EditorToolBar;
	class ViewOptionsBar;
	// エンジン機能
	class Input;
	class Model;
	class DebugCamera;
	class RenderPassController;
	class DebugRenderer;
	class RenderQueue;

	class EditorCore {
	public:
		EditorCore();
		~EditorCore();

		// 初期化処理
		void Initialize(TextureManager* textureManager, SceneChangeRequest* sceneChangeRequest, RenderPassController* renderPassController, 
			Input* input, RenderQueue* renderQueue, DebugRenderer* debugRenderer, Model* gridModel);

		// 実行
		void Run();

		// 終了処理
		void Finalize();

		// 更新状態を取得する
		bool IsActiveUpdate() const;

		bool IsPause() const;

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

		// ビュー
		std::unique_ptr<ViewOptionsBar> viewOptionsBar_;

	private:

		/// <summary>
		/// Dockをするためのスペースを作成する
		/// </summary>
		void BeginDockSpace();
	};
}

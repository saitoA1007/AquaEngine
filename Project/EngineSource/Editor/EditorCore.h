#pragma once
#include "TextureManager.h"
#include "WorldTransform.h"

class SceneChangeRequest;

namespace GameEngine {

	// 前方宣言
	class EditorWindowManager;
	class EditorMenuBar;
	class SceneMenuBar;
	class EditorLayout;
	class EditorToolBar;

	class Model;
	class RenderPassController;
	class RenderQueue;

	class EditorCore {
	public:
		EditorCore();
		~EditorCore();

		// 初期化処理
		void Initialize(TextureManager* textureManager, SceneChangeRequest* sceneChangeRequest, RenderPassController* renderPassController);

		// 実行
		void Run();

		// 更新処理
		void DebugUpdate(const Vector3& debugCameraPos);

		// 描画
		void DebugDraw(RenderQueue* renderQueue);

		// 終了処理
		void Finalize();

		// 更新状態を取得する
		bool IsActiveUpdate() const;

		bool IsPause() const;

	public:

		// グリッドモデルを取得
		void SetGridModel(Model* model);

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

		// グリッドを描画するためのモデル
		GameEngine::Model* gridModel_;
		GameEngine::WorldTransform gridWorldTransform_;

	private:

		/// <summary>
		/// Dockをするためのスペースを作成する
		/// </summary>
		void BeginDockSpace();

	};
}

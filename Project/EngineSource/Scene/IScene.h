#pragma once
#include <string>
#include "ITransitionEffect.h"
#include "Input.h"
#include "InputCommand.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include "GameObjectManager.h"
#include "RenderQueue.h"
#include "RenderPass/RenderPassController.h"

// 消す予定
#include "DebugCamera.h"

namespace GameEngine {

	/// <summary>
	/// 各シーンの元
	/// </summary>
	class IScene {
	public:
		// 入力機能を取得
		static void SetInput(Input* input, InputCommand* inputCommand) {
			input_ = input;
			inputCommand_ = inputCommand;
		}

		// リソース管理機能を取得
		static void SetResourceManager(TextureManager* textureManager,ModelManager* modelManager,
			AnimationManager* animationManager, GameObjectManager* gameObjectManager) {
			textureManager_ = textureManager;
			modelManager_ = modelManager;
			animationManager_ = animationManager;
			gameObjectManager_ = gameObjectManager;
		}

		// 描画機能を取得
		static void SetRender(RenderPassController* renderPassController, RenderQueue* renderQueue) {
			renderPassController_ = renderPassController;
			renderQueue_ = renderQueue;
		}

		// カメラ機能を取得
		static void SetMainCamera(DebugCamera* debugCamera) {
			debugCamera_ = debugCamera;
		}

		/// <summary>
		/// デストラクタ
		/// </summary>
		virtual ~IScene() = default;

		/// <summary>
		/// 初期化処理
		/// </summary>
		virtual void Initialize() = 0;

		/// <summary>
		/// 更新処理
		/// </summary>
		virtual void Update() = 0;

		/// <summary>
		/// デバック時、処理して良いものを更新する
		/// </summary>
		virtual void DebugUpdate() = 0;

		/// <summary>
		/// 描画処理
		/// </summary>
		virtual void Draw(const bool& isDebugView) = 0;

		/// <summary>
		/// 終了したことを伝える
		/// </summary>
		/// <returns></returns>
		virtual bool IsFinished() = 0;

		/// <summary>
		/// 次のシーン遷移する場面の名前を取得
		/// </summary>
		/// <returns>次のシーン名</returns>
		virtual std::string NextSceneName() = 0;

		// 遷移クラス
		virtual std::unique_ptr<ITransitionEffect> GetTransitionEffect() = 0;

	protected:
		static Input* input_; // 純粋な入力処理を取得
		static InputCommand* inputCommand_; // 登録した入力処理を取得可能

		static TextureManager* textureManager_; // 画像を取得可能
		static ModelManager* modelManager_; // モデルを取得可能
		static AnimationManager* animationManager_; // アニメーションデータを取得可能
		static GameObjectManager* gameObjectManager_; // ゲームオブジェクト監理

		static RenderPassController* renderPassController_; // 描画パスを管理する
		static RenderQueue* renderQueue_; // 描画コマンドを管理

		static DebugCamera* debugCamera_; // デバック描画機能
	};
}

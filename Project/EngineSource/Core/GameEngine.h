#pragma once
// Core
#include "WindowsApp.h"

// Core/PSO
#include "PostProcess/CopyPSO.h"
#include "PostProcess/BloomPSO.h"
#include "PSO/Core/PSOManager.h"
#include "PostProcess/PostEffectManager.h"
#include "GameObjectManager.h"

// Graphics
#include "GraphicsDevice.h"
#include "RenderPipeline.h"
#include "RenderTextureManager.h"
#include "RenderPass/RenderPassController.h"
#include "TextureManager.h"

// Common
#include "LogManager.h"
#include "ResourceLeakChecker.h"
#include "CrashHandle.h"
#include "FPSCounter.h"
#include "RandomGenerator.h"

// 3D
#include "DebugRenderer.h"
#include "RenderQueue.h"
#include "TransformationMatrix.h"

// 2D
#include "ImGuiManager.h"

// Input
#include "InPut.h"

// Editor
#ifdef USE_IMGUI
#include "EditorCore.h"
#endif
#include "GameParamEditor.h"

// Scene
#include "SceneChangeRequest.h"
#include "SceneManager.h"
#include "SceneRegistry.h"

namespace GameEngine {

	// 前方宣言
	class CollisionManager;

	class Engine final {
	public:
		Engine();
		~Engine();

		/// <summary>
		/// エンジンを実行する
		/// </summary>
		/// <param name="hInstance"></param>
		void RunEngine(HINSTANCE& hInstance);

	private:

		/// 2D ===========================================

		// ImGuiの機能
		std::unique_ptr<ImGuiManager> imGuiManager_;

		// 3D ===============================================

		// デバック描画
		std::unique_ptr<DebugRenderer> debugRenderer_;

		// モデルのリソースを管理
		std::unique_ptr<GameEngine::ModelManager> modelManager_;

		// アニメーションのリソースを管理
		std::unique_ptr<AnimationManager> animationManager_;

		// 描画処理
		std::unique_ptr<RenderQueue> renderQueue_;

		/// Collision ====================================

		// 当たり判定管理機能
		std::unique_ptr<CollisionManager> collisionManager_;

		/// Core ========================================

		// Windowsのアプリ機能
		std::unique_ptr<WindowsApp> windowsApp_;

		// dxcCompilerの機能
		std::unique_ptr<DXC> dxc_;

		// テクスチャの機能
		std::unique_ptr<TextureManager> textureManager_;

		// ゲームオブジェクト管理
		std::unique_ptr<GameObjectManager> gameObjectManager_;

		/// PSO ======================================

		/// ↓ポストエフェクト用のPSO設定

		// ポストエフェクトの内容をSwapChainに持ってくるためのPSO設定
		std::unique_ptr<CopyPSO> copyPSO_;

		// ブルーム用のPSO設定
		std::unique_ptr<BloomPSO> bloomPSO_;

		std::unique_ptr<PSOManager> psoManager_;

		// Graphics ==================================

		// DirectXのコア機能
		std::unique_ptr<GraphicsDevice> graphicsDevice_;

		// 描画の流れを管理
		std::unique_ptr<RenderPipeline> renderPipeline_;

		// ポストエフェクト
		std::unique_ptr<PostEffectManager> postEffectManager_;

		// レンダーテクスチャの管理機能
		std::unique_ptr<RenderTextureManager> renderTextureManager_;

		// レンダーパスの管理機能
		std::unique_ptr<RenderPassController> renderPassController_;

		/// Input =============================

		// 入力処理
		std::unique_ptr<Input> input_;

		// 入力処理のコマンドシステム
		std::unique_ptr<GameEngine::InputCommand> inputCommand_;

		// Editor =======================================
#ifdef USE_IMGUI
		// エディター
		std::unique_ptr<EditorCore> editorCore_;
#endif

		// 更新状態を管理
		bool isActiveUpdate_ = true;
		bool isReset = true;
		bool isPause_ = false;

		// Scene ============================================

		// ゲームシーンを管理
		std::unique_ptr<SceneManager> sceneManager_;

		// シーン切り替えの通知を管理
		std::unique_ptr<SceneChangeRequest> sceneChangeRequest_;

		// フレームレートを取得
		std::unique_ptr<FpsCounter> fpsCounter_;

		// シーンの生成機能
		std::unique_ptr<SceneRegistry> sceneRegistry_;

	private:

		/// <summary>
		/// エンジン機能の初期化
		/// </summary>
		/// <param name="title">タイトルバーの名前</param>
		/// <param name="width">画面の横幅</param>
		/// <param name="height">画面の縦幅</param>
		void Initialize(const std::wstring& title, const uint32_t& width, const uint32_t& height, HINSTANCE hInstance);

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update();

		/// <summary>
		/// 更新前処理
		/// </summary>
		void PreUpdate();

		/// <summary>
		/// 更新後処理
		/// </summary>
		void PostUpdate();

		/// <summary>
		/// 描画前処理
		/// </summary>
		void PreDraw();

		/// <summary>
		/// 描画後処理
		/// </summary>
		void PostDraw();

		/// <summary>
		/// エンジンの終了処理
		/// </summary>
		void Finalize();

		/// <summary>
		/// ウィンドウが開いているかを判断する
		/// </summary>
		/// <returns></returns>
		bool IsWindowOpen();

		/// <summary>
		/// PSOを作成
		/// </summary>
		void CreatePSO();
	};
}
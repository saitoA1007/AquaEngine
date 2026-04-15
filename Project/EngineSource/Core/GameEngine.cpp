#include "GameEngine.h"
#include "Application/Scene/Register/SetUpScenes.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")

// Audio
#include"AudioManager.h"

// Collision
#include "CollisionManager.h"
#include "Collider.h"

// Graphics
#include "GpuResource.h"
#include "SrvResource.h"

using namespace GameEngine;

Engine::Engine() {

}

Engine::~Engine() {

}

void Engine::RunEngine(HINSTANCE& hInstance) {

	// 初期化
	Initialize(L"AquaEngine", 1280, 720, hInstance);

	// 更新処理
	Update();

	// 終了処理
	Finalize();
}

void Engine::Initialize(const std::wstring& title, const uint32_t& width, const uint32_t& height, HINSTANCE hInstance) {

	// 誰も補足しなかった場合に(Unhandled)、補足する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	// ログの初期化
	LogManager::GetInstance().Create();

	// ウィンドウの作成
	windowsApp_ = std::make_unique<WindowsApp>();
	windowsApp_->CreateGameWindow(title, width, height);

	// リソースチェックのデバック
	D3DResourceLeakChecker leakCheck;

	// DirectXの機能を生成
	graphicsDevice_ = std::make_unique<GraphicsDevice>();
	graphicsDevice_->Initialize(windowsApp_->GetHwnd(), windowsApp_->kWindowWidth, windowsApp_->kWindowHeight);

	// dxcCompilerの初期化
	dxc_ = std::make_unique<DXC>();
	dxc_->Initialize();

	// PSOを作成
	CreatePSO();

	// PSOを作成
	psoManager_ = std::make_unique<PSOManager>();
	psoManager_->Initialize(graphicsDevice_->GetDevice(), dxc_.get());
	psoManager_->DefaultLoadPSO();
	psoManager_->DeaultLoadPostEffectPSO();

	GpuResource::StaticInitialize(graphicsDevice_->GetDevice());
	SrvResource::StaticInitialize(graphicsDevice_->GetSrvManager());

	// レンダーテクスチャ機能を生成
	renderTextureManager_ = std::make_unique<RenderTextureManager>();
	renderTextureManager_->Initialize(graphicsDevice_->GetRtvManager(), graphicsDevice_->GetSrvManager(), graphicsDevice_->GetDsvManager(), graphicsDevice_->GetDevice());

	// レンダーパスの管理機能
	renderPassController_ = std::make_unique<RenderPassController>();
	renderPassController_->Initialize(renderTextureManager_.get(), graphicsDevice_->GetCommandList());

	// ポストエフェクトの初期化
	PostEffectManager::StaticInitialize(bloomPSO_.get(), psoManager_.get());
	// ポストエフェクトマネージャーの初期化
	postEffectManager_ = std::make_unique<PostEffectManager>();
	float clearColor_[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	postEffectManager_->Initialize(graphicsDevice_->GetDevice(), clearColor_, windowsApp_->kWindowWidth, windowsApp_->kWindowHeight, graphicsDevice_->GetRTVDescriptorSize(), graphicsDevice_->GetSrvManager());

	// 描画の流れを管理するクラスを初期化
	renderPipeline_ = std::make_unique<RenderPipeline>();
	renderPipeline_->Initialize(graphicsDevice_.get(), postEffectManager_.get(), renderPassController_.get());
	renderPipeline_->SetCopyPSO(copyPSO_.get());

	// ImGuiの初期化
	imGuiManager_ = std::make_unique<ImGuiManager>();
	imGuiManager_->Initialize(graphicsDevice_->GetDevice(), graphicsDevice_->GetCommandList(), graphicsDevice_->GetSwapChainDesc(),
		windowsApp_.get(), graphicsDevice_->GetSrvManager());

	// 入力処理を初期化
	input_ = std::make_unique<Input>();
	input_->Initialize(hInstance, windowsApp_->GetHwnd());

	// 音声の初期化
	GameEngine::AudioManager::GetInstance().Initialize();

	// テクスチャの初期化
	textureManager_ = std::make_unique<TextureManager>();
	textureManager_->Initialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager());

	// 入力処理のコマンドシステムを生成
	inputCommand_ = std::make_unique<InputCommand>(input_.get());
	// モデルを管理するクラスを生成
	modelManager_ = std::make_unique<ModelManager>();
	modelManager_->Initialize(graphicsDevice_->GetDevice(), textureManager_.get(), graphicsDevice_->GetSrvManager());
	// アニメーションデータを管理するクラスを生成する
	animationManager_ = std::make_unique<AnimationManager>();

	// 画像の初期化
	Sprite::StaticInitialize(windowsApp_->kWindowWidth, windowsApp_->kWindowHeight);
	SpriteRenderer::StaticInitialize(graphicsDevice_->GetCommandList(), textureManager_.get(), psoManager_.get());
	ModelRenderer::StaticInitialize(graphicsDevice_->GetCommandList(), graphicsDevice_->GetSrvManager(), psoManager_.get());
	DebugRenderer::StaticInitialize(graphicsDevice_->GetCommandList(), psoManager_.get());

	// 描画コマンド管理
	renderQueue_ = std::make_unique<RenderQueue>();
	renderQueue_->Initialize(graphicsDevice_->GetCommandList(), psoManager_.get(), renderPassController_.get());

	// fpsを計測する
	fpsCounter_ = std::make_unique<FpsCounter>();
	fpsCounter_->Initialize();

	// ランダム生成器を初期化
	RandomGenerator::Initialize();

	// 全てのデバック用ファイルを読み込み
	GameParamEditor::GetInstance()->LoadFiles();

	// 当たり判管理機能を初期化
	collisionManager_ = std::make_unique<CollisionManager>();
	Collider::StaticInitialize(collisionManager_.get());

	// シーンの生成機能を初期化
	sceneRegistry_ = std::make_unique<SceneRegistry>();
	// シーンを登録する
	SetupScenes(*sceneRegistry_);

	// ゲームオブジェクト管理機能を初期化
	gameObjectManager_ = std::make_unique<GameObjectManager>();

	// ゲームシーンで使用するエンジン機能を取得
	sceneContext.input = input_.get();
	sceneContext.inputCommand = inputCommand_.get();
	sceneContext.textureManager = textureManager_.get();
	sceneContext.modelManager = modelManager_.get();
	sceneContext.graphicsDevice = graphicsDevice_.get();
	sceneContext.animationManager = animationManager_.get();
	sceneContext.gameObjectManager_ = gameObjectManager_.get();
	sceneContext.renderPassController = renderPassController_.get();
	sceneContext.renderQueue = renderQueue_.get();

	// シーンの初期化
	sceneManager_ = std::make_unique<SceneManager>();
	sceneManager_->Initialize(&sceneContext, sceneRegistry_.get());
	
	// エディターの初期化
#ifdef USE_IMGUI
	// シーン切り替えの通知を管理する機能を初期化
	sceneChangeRequest_ = std::make_unique<SceneChangeRequest>();
	sceneChangeRequest_->SetCurrentSceneName(sceneManager_->GetCurrentSceneName());
	sceneChangeRequest_->SetSceneNames(sceneRegistry_->GetSceneNames());

	editorCore_ = std::make_unique<EditorCore>();
	editorCore_->Initialize(textureManager_.get(), sceneChangeRequest_.get(), renderPassController_.get());
	editorCore_->SetGridModel(modelManager_->GetNameByModel("Grid"));
#endif
}

void Engine::Update() {
	// ウィンドウのxボタンが押されるまでループ
	while (true) {
		if (IsWindowOpen()) {
			break;
		}

		//==================================================================
		// 更新処理
		//==================================================================

		// 更新前処理
		PreUpdate();

		// シーンの更新処理
		if (isActiveUpdate_ && !isPause_) {
			// ゲームオブジェクトでおこなわれる更新処理
			gameObjectManager_->UpdateAll();
			sceneManager_->Update();

			// 当たり判定を計算
			collisionManager_->CheckAllCollisions();
		} else {
			sceneManager_->DebugSceneUpdate();
		}

		// 更新後処理
		PostUpdate();

		//====================================================================
		// 描画処理
		//====================================================================

		// 描画前処理
		PreDraw();

		// シーンの描画処理
		sceneManager_->Draw();
		// ゲームオブジェクトでおこなわれる描画処理
		gameObjectManager_->DrawAll();

		// デバック描画
		editorCore_->DebugDraw(renderQueue_.get());

		// 積まれた描画コマンドを解放する
		renderQueue_->Execute();

		// 描画後処理
		PostDraw();
	}
}

void Engine::PreUpdate() {

	// fpsを計測する
	fpsCounter_->Update();

	// キー入力の更新処理
	input_->Update();

	// 入力コマンドの更新処理
	inputCommand_->Update();

	// ImGuiにフレームが始まる旨を伝える
	imGuiManager_->BeginFrame();

#ifdef USE_IMGUI

	// エディターの処理
	editorCore_->Run();
	editorCore_->DebugUpdate(Vector3(sceneContext.debugCamera_->GetTargetPosition().x, -0.1f, sceneContext.debugCamera_->GetTargetPosition().z));

	// 更新処理の実行状態を取得する
	isActiveUpdate_ = editorCore_->IsActiveUpdate();

	// 一時停止状態を取得する
	isPause_ = editorCore_->IsPause();

	// 更新処理を管理する
	if (isActiveUpdate_) {
		if (isReset) {
			isReset = false;
		}
	} else {
		// 更新処理が停止してる時、リセットされていなければリセット
		if (!isReset) {
			sceneManager_->ResetCurrentScene();
			isReset = true;
		}
	}

	// シーン切り替えリクエストを処理
	if (sceneChangeRequest_->HasChangeRequest()) {
		// シーンを切り替える
		sceneManager_->ChangeScene(sceneChangeRequest_->GetRequestScene());
		sceneChangeRequest_->ClearChangeRequest();
		// 変更したシーンの名前を取得
		sceneChangeRequest_->SetCurrentSceneName(sceneManager_->GetCurrentSceneName());
	}

	// シーンのデバックに必要な処理を更新する
	sceneManager_->DebugUpdate();

	// ゲームオブジェクトのデバック処理を更新する
	gameObjectManager_->DebugUpdateAll();
#endif
}

void Engine::PostUpdate() {
	// ImGuiの受付終了
	imGuiManager_->EndFrame();
}

void Engine::PreDraw() {
	// 描画開始前処理
	renderQueue_->Begin();
	// 描画前処理
	renderPipeline_->BeginFrame();
}

void Engine::PostDraw() {
	// 描画後処理
	renderPipeline_->EndFrame(imGuiManager_.get());
}

void Engine::Finalize() {

	// エディターの終了処理
#ifdef USE_IMGUI
	editorCore_->Finalize();
#endif

	// 音声の終了処理
	GameEngine::AudioManager::GetInstance().Finalize();
	
	// テクスチャの解放
	textureManager_->Finalize();
	// ImGuiの解放処理
	imGuiManager_->Finalize();
	// WindowAppの解放
	windowsApp_->BreakGameWindow();
}

bool Engine::IsWindowOpen() {
	return windowsApp_->ProcessMessage();
}

void Engine::CreatePSO() {

	// CopyPSOの初期化
	copyPSO_ = std::make_unique<CopyPSO>();
	copyPSO_->Initialize(graphicsDevice_->GetDevice(), L"Resources/Shaders/PostEffect/FullScreen.VS.hlsl", L"Resources/Shaders/PostEffect/Copy.PS.hlsl", dxc_.get());

	/// PostProcessのPSOを初期化

	// BloomPSOの初期化
	bloomPSO_ = std::make_unique<BloomPSO>();
	bloomPSO_->Initialize(graphicsDevice_->GetDevice(), L"Resources/Shaders/PostEffect/Bloom.VS.hlsl", dxc_.get(),
		L"Resources/Shaders/PostEffect/HighLumMask.PS.hlsl",
		L"Resources/Shaders/PostEffect/Bloom.PS.hlsl",
		L"Resources/Shaders/PostEffect/BloomResult.PS.hlsl",
		L"Resources/Shaders/PostEffect/BloomComposite.hlsl");
}
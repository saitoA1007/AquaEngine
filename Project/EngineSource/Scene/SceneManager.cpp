#include "SceneManager.h"

#include "ImguiManager.h"
#include "AudioManager.h"
//#include"ModelRenderer.h"

using namespace GameEngine;

SceneManager::~SceneManager() {
	currentScene_.reset();
	currentScene_.release();
}

void SceneManager::Initialize(SceneContext* context, SceneRegistry* sceneRegistry) {

	// エンジン機能を取得する
	context_ = context;

	// 画像を読み込む
	LoadSpriteData();

	// モデルを読み込む
	LoadModelData();

	// アニメーションデータを読み込む
	LoadAnimationData();

	// 音声データを読み込む
	LoadAudioData();

	// シーンの生成機能を初期化
	sceneRegistry_ = sceneRegistry;

	// デバックカメラを生成
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize({ 0.0f,2.0f,-20.0f }, 1280, 720);
	context_->debugCamera_ = debugCamera_.get();
	// グリッドの初期化
	gridModel_ = context_->modelManager->GetNameByModel("Grid");
	gridWorldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	//　シーン遷移システムを初期化
	sceneTransition_ = std::make_unique<SceneTransition>();
	sceneTransition_->Initialize();

	// 使用するカメラのフラグ
#ifdef _DEBUG
	isDebugView_ = true;
#else
	isDebugView_ = false;
#endif
	
	// デフォルトシーンで初期化
	ChangeScene(sceneRegistry_->GetDefaultScene());
}

void SceneManager::Update() {

	// シーン遷移の更新処理
	sceneTransition_->Update();

	// シーンの切り替え
	if (sceneTransition_->IsMidTransition() && isChangeScene_) {
		isChangeScene_ = false;
		ChangeScene(currentScene_->NextSceneName());
	}

	// シーン遷移の開始
	if (currentScene_->IsFinished() && !isChangeScene_ && !sceneTransition_->IsActive()) {
		if (isChangeScene_ || sceneTransition_->IsActive()) {
			return;
		}
		isChangeScene_ = true;
		// トランジション開始
		sceneTransition_->Start(currentScene_->GetTransitionEffect());
	}

	// 遷移演出中でなければシーンを更新
	if (!sceneTransition_->IsActive()) {
		// 現在シーンの更新処理
		currentScene_->Update();
	}
}

void SceneManager::DebugUpdate() {

	// デバック状態を切り替える
	if (context_->input->TriggerKey(DIK_F)) {
		if (isDebugView_) {
			isDebugView_ = false;
		} else {
			isDebugView_ = true;
		}
	}

	// デバック状態で無ければ早期リターン
	if (!isDebugView_) { return; }

	// デバックカメラを操作
	debugCamera_->Update(context_->input);
	// グリッドの更新処理
	gridWorldTransform_.transform_.translate = Vector3(context_->debugCamera_->GetTargetPosition().x, -0.1f, context_->debugCamera_->GetTargetPosition().z);
	gridWorldTransform_.UpdateTransformMatrix();
}

void SceneManager::DebugSceneUpdate() {
	//　停止中でも適応される処理
	currentScene_->DebugUpdate();
}

void SceneManager::Draw() {
	// デバック用のグリッドを描画
#ifdef _DEBUG
	//if (isDebugView_) {
	//	// モデルの単体描画前処理
	//	ModelRenderer::PreDraw(RenderMode3D::Grid);
	//	// グリッドを描画
	//	ModelRenderer::DrawGrid(gridModel_, gridWorldTransform_, context_->debugCamera_->GetVPMatrix(), context_->debugCamera_->GetCameraResource());
	//}
#endif

	// 現在シーンの描画処理
	currentScene_->Draw(isDebugView_);

	// シーン遷移演出を描画
	//sceneTransition_->Draw();
}

void SceneManager::LoadModelData() {

	// グリッドモデルをロードと登録
	context_->modelManager->RegisterGridPlaneModel("Grid", { 200.0f,200.0f });

	// モデルリソースを全てロードする
	context_->modelManager->LoadAllModel();
}

void SceneManager::LoadSpriteData() {

	// テクスチャのリソースを全てロードする
	context_->textureManager->LoadAllTexture();
}

void SceneManager::LoadAnimationData() {
	// 歩くアニメーションデータを登録する
	context_->animationManager->RegisterAnimation("Walk", "walk.gltf");
}

void SceneManager::LoadAudioData() {
	GameEngine::AudioManager::GetInstance().LoadAllAudio();
}

void SceneManager::ChangeScene(const std::string& sceneName) {

	// シーン名を保存
	currentSceneName_ = sceneName;

	// 前の要素を削除
	currentScene_.reset();

	// 新しいシーンを作成
	currentScene_ = sceneRegistry_->CreateScene(sceneName);

	if (currentScene_) {
		// 新しく作ったシーンを初期化
		currentScene_->Initialize(context_);
		// 1回だけ更新処理を挟む
		currentScene_->Update();
	} else {
		// 新しいシーンのインスタンスを作れなかった場合
		assert(0 && "Scene not found");
	}
}

void SceneManager::ResetCurrentScene() {
	// 現在のシーンを再初期化する
	ChangeScene(currentSceneName_);
}
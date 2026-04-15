#include "SceneManager.h"

#include "ImguiManager.h"

using namespace GameEngine;

SceneManager::~SceneManager() {
	currentScene_.reset();
	currentScene_.release();
}

void SceneManager::Initialize(SceneRegistry* sceneRegistry) {

	// シーンの生成機能を初期化
	sceneRegistry_ = sceneRegistry;

	// デバックカメラを生成
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize({ 0.0f,2.0f,-20.0f }, 1280, 720);
	// シーンにカメラを設定
	IScene::SetMainCamera(debugCamera_.get());

	//　シーン遷移システムを初期化
	sceneTransition_ = std::make_unique<SceneTransition>();
	sceneTransition_->Initialize();

	// 使用するカメラのフラグ
#ifdef USE_IMGUI
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

	//// デバック状態を切り替える
	//if (input->TriggerKey(DIK_F)) {
	//	if (isDebugView_) {
	//		isDebugView_ = false;
	//	} else {
	//		isDebugView_ = true;
	//	}
	//}

	// デバック状態で無ければ早期リターン
	if (!isDebugView_) { return; }

	// デバックカメラを操作
	//debugCamera_->Update(context_->input);
}

void SceneManager::DebugSceneUpdate() {
	//　停止中でも適応される処理
	currentScene_->DebugUpdate();
}

void SceneManager::Draw() {

	// 現在シーンの描画処理
	currentScene_->Draw(isDebugView_);

	// シーン遷移演出を描画
	//sceneTransition_->Draw();
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
		currentScene_->Initialize();
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
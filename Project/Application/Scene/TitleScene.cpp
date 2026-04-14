#include"TitleScene.h"
#include"ImguiManager.h"
#include"ModelRenderer.h"
#include"GameParamEditor.h"
using namespace GameEngine;

TitleScene::~TitleScene() {
}

void TitleScene::Initialize(SceneContext* context) {
	// ゲームシーンに必要な低レイヤー機能
#pragma region SceneSystem 
	// エンジン機能を取得
	context_ = context;

	// 登録するパラメータを設定
	GameParamEditor::GetInstance()->SetActiveScene("TitleScene");
#pragma endregion

	// グリッドの初期化
	gridModel_ = context_->modelManager->GetNameByModel("Grid");
	gridWorldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);
}

void TitleScene::Update() {

	// カメラの更新処理
	mainCamera_->Update();
}

void TitleScene::Draw(const bool& isDebugView) {

	// 描画に使用するカメラを設定
	if (isDebugView) {
		// 描画に使用するカメラを設定
		//ModelRenderer::SetCamera(context_->debugCamera_->GetResource());
		context_->renderQueue->SetCamera(context_->debugCamera_->GetConstantBuffer());
	} else {
		// 描画に使用するカメラを設定
		//ModelRenderer::SetCamera(mainCamera_->GetResource());
		context_->renderQueue->SetCamera(mainCamera_->GetConstantBuffer());
	}

	context_->renderQueue->SubmitGrid(gridModel_, gridWorldTransform_);
}

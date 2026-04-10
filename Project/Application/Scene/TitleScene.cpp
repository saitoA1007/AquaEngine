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

	// デフォルトで描画するパス
	context_->renderPassController->AddPass("DefaultPass");
	// 最終的な描画先を設定
	context_->renderPassController->SetEndPass("DefaultPass");
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
		ModelRenderer::SetCamera(context_->debugCamera_->GetCameraResource());
	} else {
		// 描画に使用するカメラを設定
		ModelRenderer::SetCamera(mainCamera_->GetResource());
	}

	// 描画パスの管理を取得
	auto pass = context_->renderPassController;

	// 通常描画
	pass->PrePass("DefaultPass");

	// モデルの単体描画前処理
	ModelRenderer::PreDraw(RenderMode3D::Grid);
	// グリッドを描画
	ModelRenderer::DrawGrid(gridModel_, gridWorldTransform_);

	pass->PostPass("DefaultPass");

	//===========================================================
	// 3D描画
	//===========================================================

	// 3Dモデルの描画前処理
	//ModelRenderer::PreDraw(RenderMode::DefaultModel);

}

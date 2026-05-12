#include"TitleScene.h"
#include"ImguiManager.h"
#include"ModelRenderer.h"
#include"GameParamEditor.h"
using namespace GameEngine;

TitleScene::~TitleScene() {
}

void TitleScene::Initialize() {
	// ゲームシーンに必要な低レイヤー機能
#pragma region SceneSystem 

#pragma endregion

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);
	mainCamera_->Update();

	// プレイヤーモデルを生成
	model_ = modelManager_->GetNameByModel("Walk");
	model_->SetDefaultIsEnableLight(true);
	model_->SetDefaultIsEnableShadow(true);
	model_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });

	world_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	model1_ = modelManager_->GetNameByModel("Terrain");
	model1_->SetDefaultIsEnableLight(true);
	model1_->SetDefaultIsEnableShadow(true);
	model1_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	uint32_t grassGH = textureManager_->GetHandleByName("grass.png");
	model1_->SetDefaultTextureHandle(grassGH);
	world1_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,-1.0f,0.0f} });
}

void TitleScene::Update() {

	// カメラの更新処理
	mainCamera_->Update();
}

void TitleScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	renderQueue_->SubmitRaytracingModel(model_, world_);

	renderQueue_->SubmitRaytracingModel(model1_, world1_);
}

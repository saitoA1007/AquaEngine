#include"TitleScene.h"
#include"ImguiManager.h"
#include"GameParamEditor.h"
using namespace GameEngine;

TitleScene::~TitleScene() {
}

void TitleScene::Initialize() {
	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);
	mainCamera_->Update();

	// プレイヤーモデルを生成
	model_ = modelManager_->GetNameByModel("Walk");
	model_->SetDefaultIsEnableLight(true);
	model_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	world_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// 地面
	model1_ = modelManager_->GetNameByModel("Terrain");
	model1_->SetDefaultIsEnableLight(true);
	model1_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	uint32_t grassGH = textureManager_->GetHandleByName("grass.png");
	model1_->SetDefaultTextureHandle(grassGH);
	world1_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,-1.0f,0.0f} });

	// cube
	model2_ = modelManager_->GetNameByModel("Cube");
	model2_->SetDefaultIsEnableLight(true);
	model2_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	world2_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{-3.0f,1.0f,0.0f} });

	// 背景画像を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("rostock_laage_airport_4k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);
}

void TitleScene::Update() {

	// カメラの更新処理
	mainCamera_->Update();
}

void TitleScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// テストモデルを描画
	renderQueue_->SubmitRaytracingModel(model_, world_);
	renderQueue_->SubmitRaytracingModel(model1_, world1_);
	renderQueue_->SubmitRaytracingModel(model2_, world2_);
}

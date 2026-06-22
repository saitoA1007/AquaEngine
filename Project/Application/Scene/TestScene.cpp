#include "TestScene.h"
#include "ImguiManager.h"
using namespace GameEngine;

TestScene::~TestScene() {}

TestScene::TestScene() {

	inputCommand_->RegisterCommand("Decision", { {InputState::KeyTrigger, DIK_SPACE},{InputState::PadTrigger, XINPUT_GAMEPAD_X} });

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);
	mainCamera_->Update();

	// 背景画像を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// プレイヤーモデルを生成
	model_ = modelManager_->GetNameByModel("walk.gltf");
	model_->SetDefaultIsEnableLight(true);
	model_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	world_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// アニメーションデータを取得する
	walkAnimationData_ = animationManager_->GetNameByAnimations("Walk");
	// アニメーションの再生を管理する
	walkAnimator_ = std::make_unique<Animator>();
	walkAnimator_->Initialize(model_, &walkAnimationData_["Armature|mixamo.com|Layer0"]);

	// 地面
	terrainModel_ = modelManager_->GetNameByModel("terrain.obj");
	terrainModel_->SetDefaultIsEnableLight(true);
	terrainModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	uint32_t grassGH = textureManager_->GetHandleByName("grass.png");
	terrainModel_->SetDefaultTextureHandle(grassGH);
	terrainWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,-2.0f,0.0f} });

	uint32_t normalGH = textureManager_->GetHandleByName("testNormal.png");
	terrainModel_->SetDefaultNormalTexture(normalGH);

	// エフェクト用モデル
	effectModel_ = modelManager_->GetNameByModel("plane.obj");
	uint32_t effectGH = textureManager_->GetHandleByName("circle.png");
	//gameObjectManager_->AddObject<ParticleBehavior>("HitAfterEffect", 32, effectGH, effectModel_, &renderQueue_->GetMainCamera());

	// 高ポリゴン氷
	iceHighModel_ = modelManager_->GetNameByModel("ice_highPolygon.gltf");
	iceHighModel_->SetDefaultIsEnableLight(true);
	iceHighModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,0.9f });
	iceHighModel_->SetDefaultIOR(1.309f);
	iceHighWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{-4.0f,2.0f,0.0f} });
	// 中ポリゴン氷
	iceMiddleModel_ = modelManager_->GetNameByModel("ice_middlePolygon.gltf");
	iceMiddleModel_->SetDefaultIsEnableLight(true);
	iceMiddleModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,0.9f });
	iceMiddleModel_->SetDefaultIOR(1.309f);
	iceMiddleWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,2.0f,0.0f} });
	// 小ポリゴン氷
	iceLowModel_ = modelManager_->GetNameByModel("ice_lowPolygon.gltf");
	iceLowModel_->SetDefaultIsEnableLight(true);
	iceLowModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,0.9f });
	iceLowModel_->SetDefaultIOR(1.309f);
	iceLowWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{4.0f,2.0f,0.0f} });
	// キューブ氷
	iceCubeModel_ = modelManager_->GetNameByModel("cube.obj");
	iceCubeModel_->SetDefaultIsEnableLight(true);
	iceCubeModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,0.9f });
	iceCubeModel_->SetDefaultIOR(1.309f);
	iceCubeWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{8.0f,2.0f,0.0f} });

	// 氷用マテリアルを作成
	iceMaterial_.Initialize({ 1.0f,1.0f,1.0f,1.0f }, { 1.0f,1.0f,1.0f }, 500.0f, true);
	for (size_t i = 0; i < 4; ++i) {
		iceRefBuffers_[i].Create();
		iceRefBuffers_[i].SetBufferMaterial(0, iceMaterial_.GetMaterialSrvIndex());
		iceRefBuffers_[i].SetHitGroupIndex(1);
	}
}

void TestScene::Initialize() {

}

void TestScene::Update() {

	if (inputCommand_->IsCommandActive("Decision")) { isFinished_ = true; }

	// カメラの更新処理
	mainCamera_->Update();

	// アニメーションの更新処理
	walkAnimator_->Update();
}

void TestScene::DebugUpdate() {
#ifdef USE_IMGUI
	auto* light = renderQueue_->GetLightManager();

	ImGui::Begin("test");

	ImGui::DragFloat3("PlayerPos", &world_.transform_.translate.x, 0.1f);
	ImGui::DragFloat3("PlayerScale", &world_.transform_.scale.x, 0.1f);
	ImGui::ColorEdit4("PlayerColor", &playerColor_.x);

	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	ImGui::ColorEdit4("lightColor", &lightColor_.x);

	ImGui::ColorEdit4("IceColor", &color_.x);
	ImGui::DragFloat("IceRoughness", &roughness_, 0.01f,0.0f,1.0f);
	ImGui::DragFloat("IceIor", &ior_, 0.01f);
	iceMaterial_.SetColor(color_);
	iceMaterial_.SetRoughness(roughness_);
	iceMaterial_.SetIOR(ior_);

	ImGui::DragFloat3("IceHighPos", &iceHighWorld_.transform_.translate.x, 0.1f);
	ImGui::DragFloat3("IceHighScale", &iceHighWorld_.transform_.scale.x, 0.1f);
	iceHighWorld_.UpdateTransformMatrix();

	dir_.Normalize();

	light->SetDirectionalDirction(dir_);
	light->SetDirectionalIntensity(intensity_);
	light->SetDirectionalColor(lightColor_);
	world_.UpdateTransformMatrix();
	model_->SetDefaultColor(playerColor_);
	ImGui::End();
#endif
}

void TestScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 地面を描画
	renderQueue_->SubmitRaytracingModel(terrainModel_, terrainWorld_);

	// アニメーションモデル
	//renderQueue_->SubmitRaytracingModel(model_, world_);
	//
	//// それぞれの氷を描画
	//renderQueue_->SubmitRaytracingModel(iceHighModel_, iceHighWorld_, &iceRefBuffers_[0]);
	//renderQueue_->SubmitRaytracingModel(iceMiddleModel_, iceMiddleWorld_, &iceRefBuffers_[1]);
	//renderQueue_->SubmitRaytracingModel(iceLowModel_, iceLowWorld_, &iceRefBuffers_[2]);
	//renderQueue_->SubmitRaytracingModel(iceCubeModel_, iceCubeWorld_, &iceRefBuffers_[3]);
}

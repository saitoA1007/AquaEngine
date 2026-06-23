#include "TestScene.h"
#include "ImguiManager.h"
#include "PostProcess/PostEffectData.h"
using namespace GameEngine;

TestScene::~TestScene() {}

TestScene::TestScene() {

    // 決定ボタンコマンドを追加
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
	effectModel_->SetDefaultIsEnableLight(false);
	//gameObjectManager_->AddObject<ParticleBehavior>("HitAfterEffect", 32, textureManager_, effectModel_, &renderQueue_->GetMainCamera());

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
	for (size_t i = 0; i < 4; ++i) {
		iceRefBuffers_[i].Create();
		iceRefBuffers_[i].SetBufferMaterial(0, iceMaterial_.GetMaterialSrvIndex());
		iceRefBuffers_[i].SetHitGroupIndex(1);
	}

	// ディゾルブ用のテクスチャを取得
	uint32_t dissolveTexture = textureManager_->GetHandleByName("noise0.png");

	// ディゾルブテクスチャを設定
	iceMaterial_.materialData_->dissolveTextureHandle = dissolveTexture;

	// ディゾルブ用のテクスチャを設定
	auto* dissolve = postEffectManager_->GetPostEffect<Dissolve>("DissolvePass");
	if (dissolve) {
		dissolve->SetDissolveTexture(dissolveTexture);
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


	ImGui::Begin("IceMaterial");
	ImGui::ColorEdit4("IceColor", &iceMaterial_.materialData_->color.x);
	ImGui::DragFloat("IceRoughness", &iceMaterial_.materialData_->roughness, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("IceIor", &iceMaterial_.materialData_->ior, 0.01f);

	ImGui::DragFloat("IceChipScale", &iceMaterial_.materialData_->chipScale, 0.01f);
	ImGui::DragFloat("IceChipStrength", &iceMaterial_.materialData_->chipStrength, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("IceEdgeWidth", &iceMaterial_.materialData_->edgeWidth, 0.01f);
	ImGui::DragFloat("IceEdgeStrength", &iceMaterial_.materialData_->edgeStrength, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("IceMicroScale", &iceMaterial_.materialData_->microScale, 0.01f);
	ImGui::DragFloat("IceMicroStrength", &iceMaterial_.materialData_->microStrength, 0.01f);
	ImGui::DragFloat("IceDissolveThreshold", &iceMaterial_.materialData_->dissolveThreshold, 0.01f,0.0f,1.0f);
	ImGui::End();
#endif
}

void TestScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 地面を描画
	renderQueue_->SubmitRaytracingModel(terrainModel_, terrainWorld_);

	// アニメーションモデル
	renderQueue_->SubmitRaytracingModel(model_, world_);
	//
	//// それぞれの氷を描画
	renderQueue_->SubmitRaytracingModel(iceHighModel_, iceHighWorld_, &iceRefBuffers_[0]);
	renderQueue_->SubmitRaytracingModel(iceMiddleModel_, iceMiddleWorld_, &iceRefBuffers_[1]);
	renderQueue_->SubmitRaytracingModel(iceLowModel_, iceLowWorld_, &iceRefBuffers_[2]);
	renderQueue_->SubmitRaytracingModel(iceCubeModel_, iceCubeWorld_, &iceRefBuffers_[3]);
}

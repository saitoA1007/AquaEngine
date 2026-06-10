#include "TestScene.h"
#include "ImguiManager.h"
using namespace GameEngine;

TestScene::~TestScene() {}

TestScene::TestScene() {
	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);
	mainCamera_->Update();

	// 背景画像を設定rostock_laage_airport_4k
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// プレイヤーモデルを生成
	model_ = modelManager_->GetNameByModel("Walk");
	model_->SetDefaultIsEnableLight(true);
	model_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	world_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// アニメーションデータを取得する
	walkAnimationData_ = animationManager_->GetNameByAnimations("Walk");
	// アニメーションの再生を管理する
	walkAnimator_ = std::make_unique<Animator>();
	walkAnimator_->Initialize(model_, &walkAnimationData_["Armature|mixamo.com|Layer0"]);

	// 地面
	terrainModel_ = modelManager_->GetNameByModel("Terrain");
	terrainModel_->SetDefaultIsEnableLight(true);
	terrainModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	uint32_t grassGH = textureManager_->GetHandleByName("grass.png");
	terrainModel_->SetDefaultTextureHandle(grassGH);
	terrainWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,-2.0f,0.0f} });

	uint32_t normalGH = textureManager_->GetHandleByName("testNormal.png");
	terrainModel_->SetDefaultNormalTexture(normalGH);

	uint32_t effectGH = textureManager_->GetHandleByName("circle.png");
	primitiveEffect_ = std::make_unique<ParticleBehavior>("PrimitiveEffect", 16, effectGH);
	// エフェクト用モデル
	effectModel_ = modelManager_->GetNameByModel("Plane");

	// 高ポリゴン氷
	iceHighModel_ = modelManager_->GetNameByModel("Ice_highPolygon");
	iceHighModel_->SetDefaultIsEnableLight(true);
	iceHighModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,0.9f });
	iceHighModel_->SetDefaultIOR(1.309f);
	iceHighWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{-4.0f,2.0f,0.0f} });
	// 中ポリゴン氷
	iceMiddleModel_ = modelManager_->GetNameByModel("Ice_middlePolygon");
	iceMiddleModel_->SetDefaultIsEnableLight(true);
	iceMiddleModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,0.9f });
	iceMiddleModel_->SetDefaultIOR(1.309f);
	iceMiddleWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,2.0f,0.0f} });
	// 小ポリゴン氷
	iceLowModel_ = modelManager_->GetNameByModel("Ice_lowPolygon");
	iceLowModel_->SetDefaultIsEnableLight(true);
	iceLowModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,0.9f });
	iceLowModel_->SetDefaultIOR(1.309f);
	iceLowWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{4.0f,2.0f,0.0f} });
	// キューブ氷
	iceCubeModel_ = modelManager_->GetNameByModel("Cube");
	iceCubeModel_->SetDefaultIsEnableLight(true);
	iceCubeModel_->SetDefaultColor({ 1.0f,1.0f,1.0f,0.9f });
	iceCubeModel_->SetDefaultIOR(1.309f);
	iceCubeWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{8.0f,2.0f,0.0f} });
}

void TestScene::Initialize() {

}

void TestScene::Update() {

	// カメラの更新処理
	mainCamera_->Update();

	// エフェクトを更新
	primitiveEffect_->Update(mainCamera_->GetWorldMatrix());

	// アニメーションの更新処理
	walkAnimator_->Update();
}

void TestScene::DebugUpdate() {
#ifdef USE_IMGUI
	auto* light = renderQueue_->GetLightManager();

	ImGui::Begin("test");

	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	dir_.Normalize();

	light->SetDirectionalDirction(dir_);
	light->SetDirectionalIntensity(intensity_);
	ImGui::End();
#endif
}

void TestScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 地面を描画
	renderQueue_->SubmitRaytracingModel(terrainModel_, terrainWorld_);

	// それぞれの氷を描画
	renderQueue_->SubmitRaytracingModel(iceHighModel_, iceHighWorld_);
	renderQueue_->SubmitRaytracingModel(iceMiddleModel_, iceMiddleWorld_);
	renderQueue_->SubmitRaytracingModel(iceLowModel_, iceLowWorld_);
	renderQueue_->SubmitRaytracingModel(iceCubeModel_, iceCubeWorld_);

	// エフェクトを描画
	renderQueue_->SubmitInstancing(effectModel_, primitiveEffect_->GetCurrentNumInstance(), *primitiveEffect_->GetWorldTransforms(), 0.0f, BlendMode::kBlendModeAdd);
}

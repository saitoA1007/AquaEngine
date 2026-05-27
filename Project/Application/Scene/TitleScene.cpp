#include "TitleScene.h"
#include "ImguiManager.h"
using namespace GameEngine;

TitleScene::~TitleScene() {
}

TitleScene::TitleScene() {
	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);
	mainCamera_->Update();

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
	model1_ = modelManager_->GetNameByModel("Terrain");
	model1_->SetDefaultIsEnableLight(true);
	model1_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	uint32_t grassGH = textureManager_->GetHandleByName("grass.png");
	model1_->SetDefaultTextureHandle(grassGH);
	world1_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,-1.0f,0.0f} });

	uint32_t normalGH = textureManager_->GetHandleByName("testNormal.png");
	model1_->SetDefaultNormalTexture(normalGH);

	// cube
	model2_ = modelManager_->GetNameByModel("Cube");
	model2_->SetDefaultIsEnableLight(true);
	model2_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	world2_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{-4.0f,1.0f,0.0f} });
	world3_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{5.0f,-4.0f,0.0f} });

	// 背景画像を設定rostock_laage_airport_4k
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	uint32_t effectGH = textureManager_->GetHandleByName("circle.png");
	primitiveEffect_ = std::make_unique<ParticleBehavior>("PrimitiveEffect", 16, effectGH);
	// エフェクト用モデル
	effectModel_ = modelManager_->GetNameByModel("Plane");

	// リング
	ringModel_ = modelManager_->GetNameByModel("Ring");
	uint32_t lineGH = textureManager_->GetHandleByName("gradationLine.png");
	ringModel_->SetDefaultTextureHandle(lineGH);
	ringWorld_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,3.2f,0.0f},{0.0f,2.0f,0.0f} });
	ringUvTransform_ = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
}

void TitleScene::Initialize() {
	
}

void TitleScene::Update() {

	// カメラの更新処理
	mainCamera_->Update();

	// エフェクトを更新
	primitiveEffect_->Update(mainCamera_->GetWorldMatrix());

	// アニメーションの更新処理
	walkAnimator_->Update();
}

void TitleScene::DebugUpdate() {
#ifdef USE_IMGUI
	auto* light = renderQueue_->GetLightManager();

	ImGui::Begin("Ring");
	ImGui::DragFloat3("pos", &ringWorld_.transform_.translate.x);
	ImGui::DragFloat3("rotate", &ringWorld_.transform_.rotate.x);
	ImGui::DragFloat3("scale", &ringWorld_.transform_.scale.x);
	ImGui::DragFloat3("UVrotate", &ringUvTransform_.rotate.x);
	ImGui::DragFloat3("UVscale", &ringUvTransform_.scale.x);
	ImGui::End();
	ringWorld_.UpdateTransformMatrix();
	ringModel_->SetDefaultUVMatrix(ringUvTransform_);

	ImGui::Begin("test");

	ImGui::ColorEdit4("P_color", &color_.x);
	ImGui::ColorEdit4("Cubecolor", &color1_.x);
	ImGui::DragFloat("P_metalic", &metalic_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("grassMetalic", &metalic1_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("CubeMetalic", &metalic2_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("P_shininess", &roughness_, 0.1f, 0.0f, 1.0f);
	ImGui::DragFloat("CubeShininess", &roughness1_, 0.1f, 0.0f, 1.0f);
	ImGui::DragFloat("GrassShininess", &roughness2_, 0.1f, 0.0f, 1.0f);
	ImGui::DragFloat("P_IOR", &ior_, 0.1f);
	ImGui::DragFloat("CubeIOR", &ior1_, 0.1f);

	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	dir_.Normalize();

	light->SetDirectionalDirction(dir_);
	light->SetDirectionalIntensity(intensity_);

	model_->SetDefaultColor(color_);
	model2_->SetDefaultColor(color1_);
	model_->SetDefaultMetallic(metalic_);
	model1_->SetDefaultMetallic(metalic1_);
	model2_->SetDefaultMetallic(metalic2_);
	model_->SetRoughness(roughness_);
	model2_->SetRoughness(roughness1_);
	model1_->SetRoughness(roughness2_);
	model_->SetDefaultIOR(ior_);
	model2_->SetDefaultIOR(ior1_);
	ImGui::End();
#endif
}

void TitleScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// ボーンのデバック描画
	walkAnimator_->DebugDraw(debugRenderer_);

	// テストモデルを描画
	//renderQueue_->SubmitAnimation(model_, world_);
	renderQueue_->SubmitRaytracingModel(model1_, world1_);
	//renderQueue_->SubmitRaytracingModel(model2_, world2_);
	renderQueue_->SubmitModel(model2_, world3_);

	// リングを描画
	//renderQueue_->SubmitModel(ringModel_, ringWorld_);

	// エフェクトを描画
	//renderQueue_->SubmitInstancing(effectModel_, primitiveEffect_->GetCurrentNumInstance(), *primitiveEffect_->GetWorldTransforms(),0.0f, BlendMode::kBlendModeNormal);
}

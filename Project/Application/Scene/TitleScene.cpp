#include "TitleScene.h"
#include "ImguiManager.h"
#include "GameParamEditor.h"
#include "FPSCounter.h"
using namespace GameEngine;

TitleScene::~TitleScene() {
}

void TitleScene::Initialize() {

	// 移動の入力コマンドを登録する
	inputCommand_->RegisterCommand("MoveUp", { {InputState::KeyPush, DIK_W },{InputState::PadLeftStick,0,{0.0f,1.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveDown", { {InputState::KeyPush, DIK_S },{InputState::PadLeftStick,0,{0.0f,-1.0f},0.2f}, {InputState::PadPush, XINPUT_GAMEPAD_DPAD_DOWN} });
	inputCommand_->RegisterCommand("MoveLeft", { {InputState::KeyPush, DIK_A },{InputState::PadLeftStick,0,{-1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { {InputState::KeyPush, DIK_D },{InputState::PadLeftStick,0,{1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_RIGHT } });

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);
	mainCamera_->Update();

	// プレイヤーモデルを生成
	model_ = modelManager_->GetNameByModel("Walk");
	model_->SetDefaultIsEnableLight(true);
	model_->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	world_.Initialize({ {2.0f,2.0f,2.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// 歩くアニメーションデータを取得する
	walkAnimationData_ = animationManager_->GetNameByAnimations("Walk");
	// 歩くアニメーションの再生を管理する
	walkAnimator_ = std::make_unique<Animator>();
	walkAnimator_->Initialize(model_, &walkAnimationData_["Armature|mixamo.com|Layer0"]);

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
	world2_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{-4.0f,1.0f,0.0f} });
	world3_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{5.0f,-4.0f,0.0f} });

	// 背景画像を設定rostock_laage_airport_4k
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	//　地面のメッシュデータを取得
	auto& meshes = model1_->GetMeshesData();
	terrainCollision_.Build(*meshes[0]);
	terrainCollision_.BuildGrid(2.0f);

	// 1000回
	players.resize(8000);
	for (uint32_t i = 0; i < players.size(); ++i) {
		players[i] = { { 1.0f, 10.0f, 2.0f }, -1.0f };
	}
}

void TitleScene::Update() {

	// カメラの更新処理
	mainCamera_->Update();

	Vector3 move = { 0.0f,0.0f,0.0f };
	Vector3 velocity_ = { 0.0f,0.0f,0.0f };
	float speed = 5.0f;
	bool isMove = false;

	world_.transform_.translate.y -= 8.0f * FpsCounter::deltaTime;

	if (inputCommand_->IsCommandAcitve("MoveUp")) {
		move.z = 1.0f;
		isMove = true;
	}

	if (inputCommand_->IsCommandAcitve("MoveDown")) {
		move.z = -1.0f;
		isMove = true;
	}

	if (inputCommand_->IsCommandAcitve("MoveLeft")) {
		move.x = -1.0f;
		isMove = true;
	}

	if (inputCommand_->IsCommandAcitve("MoveRight")) {
		move.x = 1.0f;
		isMove = true;
	}

	if (isMove) {
		// 正規化する
		move = Normalize(move);
		move = TransformNormal(move, renderQueue_->debugCam_->GetRotateMatrix());
		move.y = 0.0f;
		move = Normalize(move);
		// 移動する
		velocity_ = move * speed;
	}

	world_.transform_.translate += velocity_ * FpsCounter::deltaTime;

	// 位置を設定
	for (int i = 0; i < players.size(); ++i) {
		players[i].position = world_.transform_.translate;
	}

	// メッシュとの当たり判定を取得
	terrainCollision_.ResolveCollision(players);

	//for (int i = 0; i < players.size(); ++i) {
	//	
	//}

	// 修正位置を取得
	world_.transform_.translate = players[0].position;

	world_.UpdateTransformMatrix();

	// アニメーションの更新処理
	walkAnimator_->ComputeUpdate();

#ifdef USE_IMGUI
	auto* light = renderQueue_->GetLightManager();

	ImGui::Begin("test");

	ImGui::ColorEdit4("color", &color_.x);
	ImGui::ColorEdit4("Cubecolor", &color1_.x);
	ImGui::DragFloat("P_metalic", &metalic_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("grassMetalic", &metalic1_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("CubeMetalic", &metalic2_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("P_shininess", &shininess_,0.1f, 0.0f, 1.0f);
	ImGui::DragFloat("CubeShininess", &shininess1_,0.1f, 0.0f, 1.0f);
	ImGui::DragFloat("GrassShininess", &shininess2_,0.1f, 0.0f, 1.0f);
	ImGui::DragFloat("P_IOR", &ior_, 0.1f);
	ImGui::DragFloat("CubeIOR", &ior1_, 0.1f);

	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	dir_ = Normalize(dir_);

	light->SetDirectionalDirction(dir_);
	light->SetDirectionalIntensity(intensity_);

	model_->SetDefaultColor(color_);
	model2_->SetDefaultColor(color1_);
	model_->SetDefaultMetallic(metalic_);
	model1_->SetDefaultMetallic(metalic1_);
	model2_->SetDefaultMetallic(metalic2_);
	model_->SetRoughness(shininess_);
	model2_->SetRoughness(shininess1_);
	model1_->SetRoughness(shininess2_);
	model_->SetDefaultIOR(ior_);
	model2_->SetDefaultIOR(ior1_);
	ImGui::End();
#endif
}

void TitleScene::DebugUpdate() {
#ifdef USE_IMGUI
	auto* light = renderQueue_->GetLightManager();

	ImGui::Begin("test");

	ImGui::ColorEdit4("color", &color_.x);
	ImGui::ColorEdit4("Cubecolor", &color1_.x);
	ImGui::DragFloat("P_metalic", &metalic_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("grassMetalic", &metalic1_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("CubeMetalic", &metalic2_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("P_shininess", &shininess_, 0.1f, 0.0f, 1.0f);
	ImGui::DragFloat("CubeShininess", &shininess1_, 0.1f, 0.0f, 1.0f);
	ImGui::DragFloat("GrassShininess", &shininess2_, 0.1f, 0.0f, 1.0f);
	ImGui::DragFloat("P_IOR", &ior_, 0.1f);
	ImGui::DragFloat("CubeIOR", &ior1_, 0.1f);

	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	dir_ = Normalize(dir_);

	light->SetDirectionalDirction(dir_);
	light->SetDirectionalIntensity(intensity_);

	model_->SetDefaultColor(color_);
	model2_->SetDefaultColor(color1_);
	model_->SetDefaultMetallic(metalic_);
	model1_->SetDefaultMetallic(metalic1_);
	model2_->SetDefaultMetallic(metalic2_);
	model_->SetRoughness(shininess_);
	model2_->SetRoughness(shininess1_);
	model1_->SetRoughness(shininess2_);
	model_->SetDefaultIOR(ior_);
	model2_->SetDefaultIOR(ior1_);
	ImGui::End();
#endif
}

void TitleScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// テストモデルを描画
	renderQueue_->SubmitRaytracingModel(model_, world_);
	renderQueue_->SubmitRaytracingModel(model1_, world1_);
	//renderQueue_->SubmitRaytracingModel(model2_, world2_);
	renderQueue_->SubmitModel(model2_, world3_);
}

#include "GameScene.h"
#include "ImguiManager.h"
#include "ModelRenderer.h"

#include "GameParamEditor.h"
#include "FPSCounter.h"
#include "LogManager.h"

// アプリ機能
#include "Application/Player/Player.h"

using namespace GameEngine;

GameScene::~GameScene() {
}

void GameScene::Initialize() {
	// ゲームシーンに必要な低レイヤー機能
#pragma region SceneSystem

	// 登録するパラメータを設定
	GameParamEditor::GetInstance()->SetActiveScene("GameScene");
#pragma endregion

	InputRegisterCommand();

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);

	// 太陽によるカメラの位置を設定
	directionLightCamera_ = std::make_unique<Camera>();
	directionLightCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.6f,0.0f,0.0f},{0.0f,22.0f,-32.0f} }, 1280, 720);

	// 平行光源ライト
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(true, false, false);
	directionalData_.active = true;
	directionalData_.color = { 1.0f,1.0f,1.0f,1.0f };
	directionalData_.direction = { 0.0,-1.0f,0.0f };
	directionalData_.intensity = 1.0f;
	directionalData_.isDepthTexture = renderPassController_->GetSrvIndex("ShadowPass");
	lightManager_->SetDirectionalData(directionalData_);
	lightManager_->Setshadow({ 0.0f,0.0f,0.0f }, 60.0f);

	// 平行光源の位置のカメラ行列を取得する
	directionLightCamera_->SetVPMatrix(lightManager_->directionalLight_->directionalLightData_.vpMatrix);

	// 地面モデルを生成
	terrainModel_ = modelManager_->GetNameByModel("Terrain");
	terrainModel_->SetDefaultIsEnableLight(true);
	terrainModel_->SetDefaultIsEnableShadow(true);
	grassGH_ = textureManager_->GetHandleByName("grass.png");
	terrainModel_->SetDefaultTextureHandle(grassGH_);
	terrainWorldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,-1.6f,0.0f},{0.0f,-1.0f,0.0f} });

	// スカイボックスの生成
	skyboxModel_ = modelManager_->GetNameByModel("Skybox");
	skyboxWorldTransform_.Initialize({ {100.0f,100.0f,100.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });
	skyboxGH_ = textureManager_->GetHandleByName("rostock_laage_airport_4k.dds");
	skyboxModel_->SetDefaultTextureHandle(skyboxGH_);
	lightManager_->SetEnvironmentTexture(skyboxGH_);

	// プレイヤーモデルを生成
	auto playerModel_ = modelManager_->GetNameByModel("Cube");
	playerModel_->SetDefaultIsEnableLight(true);
	playerModel_->SetDefaultIsEnableShadow(true);
	// プレイヤークラスを初期化
	gameObjectManager_->AddObject<Player>(inputCommand_, playerModel_);

	// 平面モデルを生成
	planeModel_ = modelManager_->GetNameByModel("Plane");
	planeModel_->SetDefaultIsEnableLight(true);
	uvCheckerGH_ = textureManager_->GetHandleByName("uvChecker.png");
	planeWorldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,1.0f,0.0f} });

	// ボーンアニメーションを生成する
	boneAnimationModel_ = modelManager_->GetNameByModel("Walk");
	boneAnimationWorldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// 歩くアニメーションデータを取得する
	walkAnimationData_ = animationManager_->GetNameByAnimations("Walk");
	// 歩くアニメーションの再生を管理する
	walkAnimator_ = std::make_unique<Animator>();
	walkAnimator_->Initialize(boneAnimationModel_, &walkAnimationData_["Armature|mixamo.com|Layer0"]);
	
	// 値の保存の登録と適応(テスト)
	RegisterDebugParam();
	ApplyDebugParam();
}

void GameScene::Update() {

	ApplyDebugParam();

	// 地面の更新処理
	terrainWorldTransform_.UpdateTransformMatrix();

	// ライトの更新
	lightManager_->Update();

	// 歩くアニメーションの更新処理
	walkAnimator_->Update();

	// カメラの更新処理
	mainCamera_->Update();

	//directionLightCamera_->Update();

#ifdef USE_IMGUI

	// 光源をデバック
	ImGui::Begin("DebugWindow");
	// カメラのデバック
	ImGui::DragFloat3("CameraTranslate", &mainCamera_->transform_.translate.x, 0.01f);
	ImGui::DragFloat3("CameraRotate", &mainCamera_->transform_.rotate.x, 0.01f);

	//ImGui::DragFloat3("DCameraTranslate", &directionLightCamera_->transform_.translate.x, 0.01f);
	//ImGui::DragFloat3("DCameraRotate", &directionLightCamera_->transform_.rotate.x, 0.01f);

	// 平行光源
	if (ImGui::TreeNodeEx("Light", ImGuiTreeNodeFlags_Framed)) {
		ImGui::DragFloat3("Dirction", &directionalData_.direction.x, 0.01f);
		directionalData_.direction = Normalize(directionalData_.direction);
		ImGui::DragFloat("Intensity", &directionalData_.intensity, 0.01f);
		ImGui::ColorEdit3("Color", &directionalData_.color.x);
		lightManager_->SetDirectionalData(directionalData_);
		ImGui::TreePop();
	}
	ImGui::End();
#endif

	// カメラの位置を更新
	lightManager_->Setshadow(player_->GetPlayerPos(), 60.0f);
	directionLightCamera_->SetVPMatrix(lightManager_->directionalLight_->directionalLightData_.vpMatrix);
}

void GameScene::Draw() {

	// 太陽の位置のカメラを設定
	renderQueue_->SetLightCamera(directionLightCamera_->GetResource());
	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_->GetConstantBuffer());

	//// 描画パスの管理を取得
	//auto pass = context_->renderPassController;
	//
	//// 太陽の位置のカメラ設定を
	//ModelRenderer::SetCamera(directionLightCamera_->GetResource());
	//
	//// 影を描画するためのパス
	//pass->PrePass("ShadowPass");
	//
	//// shadowMapの描画
	//ModelRenderer::PreDraw(RenderMode3D::ShadowMap);
	//
	//// 地面を描画
	//ModelRenderer::DrawShadowMap(terrainModel_, terrainWorldTransform_);
	//
	//// プレイヤーを描画
	//ModelRenderer::DrawShadowMap(playerModel_, player_->GetWorldTransform());
	//
	//pass->PostPass("ShadowPass");

	//// 通常描画
	//pass->PrePass("DefaultPass");
	//
	////===========================================================
	//// 3D描画
	////===========================================================
	//
	//// スカイボックスの描画前処理
	//ModelRenderer::PreDraw(RenderMode3D::Skybox);
	//
	//ModelRenderer::DrawSkybox(skyboxModel_, skyboxWorldTransform_);
	//
	//// 3Dモデルの描画前処理
	//ModelRenderer::PreDraw(RenderMode3D::DefaultModel);
	//
	//// 地面を描画
	//ModelRenderer::DrawLight(lightManager_->GetConstantBuffer());
	//ModelRenderer::Draw(terrainModel_, terrainWorldTransform_);
	//
	//// プレイヤーを描画
	//ModelRenderer::DrawLight(lightManager_->GetConstantBuffer());
	//ModelRenderer::Draw(playerModel_, player_->GetWorldTransform());
	//
	//// アニメーションの描画前処理
	////ModelRenderer::PreDraw(RenderMode3D::AnimationModel);
	//
	//// アニメーションしているモデルを描画
	////ModelRenderer::DrawAnimation(boneAnimationModel_, boneAnimationWorldTransform_);
	//
	//pass->PostPass("DefaultPass");
}

void GameScene::InputRegisterCommand() {
	// 移動の入力コマンドを登録する
	inputCommand_->RegisterCommand("MoveUp", { {InputState::KeyPush, DIK_W },{InputState::PadLeftStick,0,{0.0f,1.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveDown", { {InputState::KeyPush, DIK_S },{InputState::PadLeftStick,0,{0.0f,-1.0f},0.2f}, {InputState::PadPush, XINPUT_GAMEPAD_DPAD_DOWN} });
	inputCommand_->RegisterCommand("MoveLeft", { {InputState::KeyPush, DIK_A },{InputState::PadLeftStick,0,{-1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { {InputState::KeyPush, DIK_D },{InputState::PadLeftStick,0,{1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_RIGHT } });
	// ジャンプコマンドを登録する
	inputCommand_->RegisterCommand("Jump", { {InputState::KeyTrigger, DIK_SPACE},{InputState::PadTrigger, XINPUT_GAMEPAD_A} });

	// カメラ操作のコマンドを登録する
	inputCommand_->RegisterCommand("CameraMoveLeft", { { InputState::KeyPush, DIK_LEFT },{InputState::PadRightStick,0,{-1.0f,0.0f},0.2f} });
	inputCommand_->RegisterCommand("CameraMoveRight", { { InputState::KeyPush, DIK_RIGHT },{InputState::PadRightStick,0,{1.0f,0.0f},0.2f} });
}


void GameScene::RegisterDebugParam() {
	GameParamEditor::GetInstance()->AddItem("Test1", "testNum", testNumber);
	GameParamEditor::GetInstance()->AddItem("Test2", "testVec", testVector);
}

void GameScene::ApplyDebugParam() {
	testNumber = GameParamEditor::GetInstance()->GetValue<float>("Test1", "testNum");
	testVector = GameParamEditor::GetInstance()->GetValue<Vector3>("Test2", "testVec");
}
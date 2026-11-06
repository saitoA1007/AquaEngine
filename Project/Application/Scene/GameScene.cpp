#include "pch.h"
#include "GameScene.h"
#include "ImguiManager.h"
using namespace GameEngine;

#include "PostProcess/PostEffectData.h"

#include "Application/Player/Player.h"
#include "Application/Player/PlayerEffectManager.h"

#include "Application/Stage/StageManager.h"
#include "Application/Stage/BgIceRock.h"

#include "Application/Enemy/BossEnemy.h"
#include "Application/Enemy/BossRangedAttackManager.h"

#include "Application/GamePlay/GamePhaseManager.h"

#include "Application/UI/Managers/TitleUIManager.h"
#include "Application/UI/Managers/PlayUIManager.h"
#include "Application/UI/Managers/GameOverUIManager.h"
#include "Application/UI/Managers/ClearUIManager.h"
#include "Application/UI/Managers/PauseUIManager.h"
#include "Application/UI/Managers/EnterMovieUIManager.h"

GameScene::~GameScene() {
}

GameScene::GameScene() {
	// 入力コマンド設定s
	InputRegisterCommand();

	// 背景を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("starrySky.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);
	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// プレイヤーエフェクト管理
	auto* playerEffectManager = gameObjectManager_->AddObject<PlayerEffectManager>(gameObjectManager_, modelManager_, textureManager_, effectsManager_);

	// プレイヤー
	auto* playerModel = modelManager_->GetNameByModel("PlayerRush.gltf");
	playerModel->SetDefaultIsEnableLight(true);
	auto player = gameObjectManager_->AddObject<Player>(inputCommand_, playerModel, animationManager_, playerEffectManager);

	// 敵の遠距離攻撃管理
	auto* iceFallModel = modelManager_->GetNameByModel("iceFall.obj");
	auto* iceFallFractureModel = modelManager_->GetNameByModel("iceFallFracture.gltf");
	auto* windModel = modelManager_->GetNameByModel("wind.obj");
	windModel->SetDefaultColor({ 1.0f,1.0f,1.0f,1.0f });
	auto* rushModel = modelManager_->GetNameByModel("rushWave.obj");
	rushModel->SetDefaultIsEnableLight(false);
	auto* bossRangedAttackManager = gameObjectManager_->AddObject<BossRangedAttackManager>(gameObjectManager_, iceFallModel, iceFallFractureModel,
		textureManager_, windModel, rushModel);

	// 敵
	auto* enemyModel = modelManager_->GetNameByModel("BossBird.gltf");
	enemyModel->SetDefaultIsEnableLight(true);
	auto* eggModel = modelManager_->GetNameByModel("BossEgg.obj");
	auto bossEnemy = gameObjectManager_->AddObject<BossEnemy>(enemyModel, eggModel, player->GetWorldTransform(), animationManager_, bossRangedAttackManager);
	bossEnemy->SetPlayer(player);

	// カメラ操作
	cameraController_ = gameObjectManager_->AddObject<CameraController>(inputCommand_, &bossEnemy->GetWorldTransform(), &player->GetWorldTransform());
	player->SetCamera(cameraController_);

	// ステージ
	auto* stageManager = gameObjectManager_->AddObject<StageManager>(gameObjectManager_, modelManager_, textureManager_, cameraController_);

	// 仮の背景の氷オブジェクト。後でオブジェクト設置エディターでマテリアルを変更出来るようにしておく。
	auto* bgIceRockModel = modelManager_->GetNameByModel("BGIceRock.obj");
	gameObjectManager_->AddObject<BgIceRock>(bgIceRockModel);

	// ステージに降っている雪を描画
	auto* planeModel = modelManager_->GetNameByModel("plane.obj");
	planeModel->SetDefaultIsEnableLight(false);
	gameObjectManager_->AddObject<ParticleBehavior>("BgSnowParticle", 64, textureManager_, planeModel);
	// 床の近くを流れる吹雪
	auto* crossPlaneModel = modelManager_->GetNameByModel("crossPlane.gltf");
	gameObjectManager_->AddObject<ParticleBehavior>("BgDriftSnowParticle", 160, textureManager_, planeModel);

	// タイトル中のUI
	auto* titleUIManager = gameObjectManager_->AddObject<TitleUIManager>(textureManager_);
	// プレイ中のUI
	auto* playUIManager = gameObjectManager_->AddObject<PlayUIManager>(textureManager_, planeModel);
	// ゲームオーバーのUI
	auto* gameOverUIManager = gameObjectManager_->AddObject<GameOverUIManager>(textureManager_);
	// クリアのUI
	auto* clearUIManager = gameObjectManager_->AddObject<ClearUIManager>(textureManager_);
	// ポーズのUI
	auto* pauseUIManager = gameObjectManager_->AddObject<PauseUIManager>(textureManager_);
	// 入りのムービーのUI
	auto* enterMovieUIManager = gameObjectManager_->AddObject<EnterMovieUIManager>(fontManager_, textureManager_);

	// 遷移する用のテクスチャを設定
	Dissolve* dissolve = postEffectManager_->GetPostEffect<Dissolve>("Dissolve");
	dissolve->SetNoiseTextureIndex(textureManager_->GetHandleByName("noise0.png"));

	// シーンフェーズを管理
	gameObjectManager_->AddObject<GamePhaseManager>(inputCommand_, player, bossEnemy, titleUIManager, playUIManager, gameOverUIManager, clearUIManager,
		pauseUIManager, cameraController_, dissolve, stageManager, enterMovieUIManager);

	// ライトを設定
	auto* light = renderQueue_->GetLightManager();
	light->directionalLight_->directionalLightData_.intensity = 0.9f;
}

void GameScene::Initialize() {

	
}

void GameScene::Update() {

	// カメラの更新処理
	//mainCamera_->Update();

	mainCamera_->SetCamera(cameraController_->GetCamera());
#ifdef USE_IMGUI
	auto* light =  renderQueue_->GetLightManager();

	ImGui::Begin("test");
	ImGui::DragFloat("LightIntensity", &light->directionalLight_->directionalLightData_.intensity, 0.01f);
	ImGui::ColorEdit3("LightIntensity", &light->directionalLight_->directionalLightData_.color.x);
	ImGui::End();
#endif
}

void GameScene::Draw() {
	
}

void GameScene::InputRegisterCommand() {

	// 決定ボタン
	inputCommand_->RegisterCommand("PauseAction", { KeyBinding{ DIK_M, PressType::kTrigger },PadButtonBinding{ XINPUT_GAMEPAD_START, PressType::kTrigger } });
	inputCommand_->RegisterCommand("Decision", { KeyBinding{ DIK_SPACE, PressType::kTrigger },PadButtonBinding{ XINPUT_GAMEPAD_A, PressType::kTrigger } });
	// 入りのムービーを飛ばす
	inputCommand_->RegisterCommand("SkipMovie", { KeyBinding{ DIK_SPACE, PressType::kTrigger },PadButtonBinding{ XINPUT_GAMEPAD_A, PressType::kTrigger } });
	inputCommand_->RegisterCommand("SelectUp", { KeyBinding{ DIK_W, PressType::kTrigger },PadStickBinding{ PadSide::kLeft, {0.0f,1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_UP, PressType::kTrigger } });
	inputCommand_->RegisterCommand("SelectDown", { KeyBinding{ DIK_S, PressType::kTrigger },PadStickBinding{ PadSide::kLeft, {0.0f,-1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_DOWN, PressType::kTrigger } });

	// 移動の入力コマンドを登録する
	inputCommand_->RegisterCommand("MoveUp", { KeyBinding{ DIK_W },PadStickBinding{ PadSide::kLeft, {0.0f,1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveDown", { KeyBinding{ DIK_S },PadStickBinding{ PadSide::kLeft, {0.0f,-1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_DOWN } });
	inputCommand_->RegisterCommand("MoveLeft", { KeyBinding{ DIK_A },PadStickBinding{ PadSide::kLeft, {-1.0f,0.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { KeyBinding{ DIK_D },PadStickBinding{ PadSide::kLeft, {1.0f,0.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_RIGHT } });
	// ジャンプコマンドを登録する
	inputCommand_->RegisterCommand("Jump", { KeyBinding{ DIK_SPACE, PressType::kTrigger },PadButtonBinding{ XINPUT_GAMEPAD_A, PressType::kTrigger } });
	
	// カメラ操作のコマンドを登録する
	inputCommand_->RegisterCommand("CameraMoveLeft", { KeyBinding{ DIK_LEFT },PadStickBinding{ PadSide::kRight, {-1.0f,0.0f} } });
	inputCommand_->RegisterCommand("CameraMoveRight", { KeyBinding{ DIK_RIGHT },PadStickBinding{ PadSide::kRight, {1.0f,0.0f} } });
	inputCommand_->RegisterCommand("CameraMoveUp", { KeyBinding{ DIK_UP },PadStickBinding{ PadSide::kRight, {0.0f,1.0f} } });
	inputCommand_->RegisterCommand("CameraMoveDown", { KeyBinding{ DIK_DOWN },PadStickBinding{ PadSide::kRight, {0.0f,-1.0f} } });
	// ロックオン
	inputCommand_->RegisterCommand("CameraLockOn", { KeyBinding{ DIK_L, PressType::kTrigger },PadTriggerBinding{ PadSide::kRight, PressType::kTrigger },PadTriggerBinding{ PadSide::kLeft, PressType::kTrigger } });

	// AttackDownコマンド
	inputCommand_->RegisterCommand("AttackDown", { MouseBinding{ 1, PressType::kTrigger }, PadButtonBinding{ XINPUT_GAMEPAD_X, PressType::kTrigger } });
	// RushChargeコマンド
	inputCommand_->RegisterCommand("RushCharge", { MouseBinding{ 1, PressType::kTrigger }, PadButtonBinding{ XINPUT_GAMEPAD_X, PressType::kTrigger } });
	// RushStartコマンド
	inputCommand_->RegisterCommand("RushStart", { MouseBinding{ 1, PressType::kRelease }, PadButtonBinding{ XINPUT_GAMEPAD_X, PressType::kRelease } });
}
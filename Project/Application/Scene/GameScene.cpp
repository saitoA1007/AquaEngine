#include "GameScene.h"
using namespace GameEngine;

#include "Application/Player/Player.h"
#include "Application/Stage/StageManager.h"
#include "Application/Enemy/BossEnemy.h"

GameScene::~GameScene() {
}

GameScene::GameScene() {
	// 入力コマンド設定
	InputRegisterCommand();

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);
	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// プレイヤー
	auto* playerModel = modelManager_->GetNameByModel("Walk");
	playerModel->SetDefaultIsEnableLight(true);
	auto player = gameObjectManager_->AddObject<Player>(inputCommand_, playerModel);

	// 敵
	auto* enemyModel = modelManager_->GetNameByModel("Cube");
	enemyModel->SetDefaultIsEnableLight(true);
	auto bossEnemy = gameObjectManager_->AddObject<BossEnemy>(enemyModel, player->GetWorldTransform());

	// カメラ操作
	cameraController_ = gameObjectManager_->AddObject<CameraController>(inputCommand_, &player->GetWorldTransform(), &bossEnemy->GetWorldTransform());
	player->SetCamera(cameraController_);

	// ステージ
	auto* wallModel = modelManager_->GetNameByModel("Cube");
	gameObjectManager_->AddObject<StageManager>(gameObjectManager_, wallModel);
}

void GameScene::Initialize() {

	
}

void GameScene::Update() {

	// カメラの更新処理
	//mainCamera_->Update();

	mainCamera_->SetCamera(cameraController_->GetCamera());
}

void GameScene::Draw() {
	
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
	// ロックオン
	inputCommand_->RegisterCommand("CameraLockOn", { {InputState::KeyTrigger, DIK_L},{InputState::PadTriggerRightTrigger,0,{0.0f,0.0f},0.2f},{InputState::PadTriggerLeftTrigger,0,{0.0f,0.0f},0.2f} });

	// 決定ボタン

	// AttackDownコマンド
	inputCommand_->RegisterCommand("AttackDown", { {InputState::MouseTrigger, 1}, {InputState::PadTrigger, XINPUT_GAMEPAD_X} });
	// RushChargeコマンド
	inputCommand_->RegisterCommand("RushCharge", { {InputState::MouseTrigger, 1}, {InputState::PadTrigger, XINPUT_GAMEPAD_X} });
	// RushStartコマンド
	inputCommand_->RegisterCommand("RushStart", { {InputState::MouseRelease, 1}, {InputState::PadRelease, XINPUT_GAMEPAD_X} });
}
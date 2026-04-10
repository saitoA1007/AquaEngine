#include"GEScene.h"
#include"ImguiManager.h"
#include"ModelRenderer.h"
#include"SpriteRenderer.h"
#include"GameParamEditor.h"
#include"LogManager.h"
#include<numbers>
using namespace GameEngine;

GEScene::~GEScene() {
}

void GEScene::Initialize(SceneContext* context) {
	// ゲームシーンに必要な低レイヤー機能
#pragma region SceneSystem 
	// エンジン機能を取得
	context_ = context;

	// デバック用描画の初期化
	debugRenderer_ = DebugRenderer::Create();

	// 登録するパラメータを設定
	GameParamEditor::GetInstance()->SetActiveScene("GEScene");

	// デフォルトで描画するパス
	context->renderPassController->AddPass("DefaultPass");
	// 最終的な描画先を設定
	context_->renderPassController->SetEndPass("DefaultPass");

#pragma endregion

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);

	// プレイヤーモデルを生成
	playerModel_ = context_->modelManager->GetNameByModel("Cube");
	// プレイヤークラスを初期化
	player_ = std::make_unique<Player>();
	player_->Initialize();

	// カメラをコントロールするクラスを初期化
	cameraController_ = std::make_unique<CameraController>();
	cameraController_->Initialize();

	// 画像
	sprite_ = Sprite::Create({0.0f,0.0f},{256.0f,256.0f},{0.0f,0.0f});
	// uvCheckerの画像を取得
	uvCheckerGH_ = context_->textureManager->GetHandleByName("uvChecker.png");

	// パーティクルのシステムを初期化
	testParticle_ = std::make_unique<ParticleBehavior>();
	testParticle_->Initialize("testParticle",16, uvCheckerGH_);
	testParticle_->Emit({ 0.0f,0.0f,0.0f });

	// 平面モデル
	planeModel_ = context_->modelManager->GetNameByModel("Plane");

	// ログのテスト
	Log("HallWorldConsloe","test");
	Log("HallWorldConsloe");

	// 入力コマンドを設定する
	InputRegisterCommand();
}

void GEScene::Update() {

	// デバックリストを削除
	debugRenderer_->Clear();

	// プレイヤーの更新処理
	player_->Update(context_->inputCommand);

	// パーティクルの更新処理
	testParticle_->Update(mainCamera_->GetWorldMatrix());

	// カメラコントロールの更新処理
	cameraController_->Update(context_->inputCommand,player_->GetPlayerPos());

	// カメラの更新処理
	mainCamera_->SetCamera(cameraController_->GetCamera());
}

void GEScene::Draw(const bool& isDebugView) {

	// 描画に使用するカメラを設定
	if (isDebugView) {
		// 描画に使用するカメラを設定
		ModelRenderer::SetCamera(context_->debugCamera_->GetVPMatrix(), context_->debugCamera_->GetCameraResource());
	} else {
		// 描画に使用するカメラを設定
		ModelRenderer::SetCamera(mainCamera_->GetVPMatrix(), mainCamera_->GetCameraResource());
	}

	// 描画パスの管理を取得
	auto pass = context_->renderPassController;

	// 通常描画
	pass->PrePass("DefaultPass");

	//===========================================================
	// 3D描画
	//===========================================================

	// 複数モデルの描画前処理
	ModelRenderer::PreDraw(RenderMode3D::Instancing);

	// パーティクルを描画
	ModelRenderer::DrawInstancing(planeModel_, testParticle_->GetCurrentNumInstance(), *testParticle_->GetWorldTransforms());

	// 3Dモデルの描画前処理
	ModelRenderer::PreDraw(RenderMode3D::DefaultModel);

	// プレイヤーを描画
	ModelRenderer::Draw(playerModel_, player_->GetWorldTransform());

#ifdef _DEBUG

	// デバック描画
	if (isDebugView) {
		debugRenderer_->DrawAll(context_->debugCamera_->GetVPMatrix());
	} 
#endif

	//========================================================================
	// 2D描画
	//========================================================================

	// 画像の描画前処理
	SpriteRenderer::PreDraw(RenderMode2D::Normal);

	SpriteRenderer::Draw(sprite_.get(), uvCheckerGH_);

	pass->PostPass("DefaultPass");
}

void GEScene::InputRegisterCommand() {
	// 移動の入力コマンドを登録する
	context_->inputCommand->RegisterCommand("MoveUp", { {InputState::KeyPush, DIK_W },{InputState::PadLeftStick,0,{0.0f,1.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_UP } });
	context_->inputCommand->RegisterCommand("MoveDown", { {InputState::KeyPush, DIK_S },{InputState::PadLeftStick,0,{0.0f,-1.0f},0.2f}, {InputState::PadPush, XINPUT_GAMEPAD_DPAD_DOWN} });
	context_->inputCommand->RegisterCommand("MoveLeft", { {InputState::KeyPush, DIK_A },{InputState::PadLeftStick,0,{-1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_LEFT } });
	context_->inputCommand->RegisterCommand("MoveRight", { {InputState::KeyPush, DIK_D },{InputState::PadLeftStick,0,{1.0f,0.0f},0.2f}, { InputState::PadPush, XINPUT_GAMEPAD_DPAD_RIGHT } });
	// ジャンプコマンドを登録する
	context_->inputCommand->RegisterCommand("Jump", { {InputState::KeyTrigger, DIK_SPACE},{InputState::PadTrigger, XINPUT_GAMEPAD_A} });

	// カメラ操作のコマンドを登録する
	context_->inputCommand->RegisterCommand("CameraMoveLeft", { { InputState::KeyPush, DIK_LEFT },{InputState::PadRightStick,0,{-1.0f,0.0f},0.2f} });
	context_->inputCommand->RegisterCommand("CameraMoveRight", { { InputState::KeyPush, DIK_RIGHT },{InputState::PadRightStick,0,{1.0f,0.0f},0.2f} });
}

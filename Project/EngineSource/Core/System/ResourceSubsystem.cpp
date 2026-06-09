#include "ResourceSubsystem.h"
#include "DebugParameter.h"
#include "GraphicsSubsystem.h"
using namespace GameEngine;

void ResourceSubsystem::Initialize() {
	auto* cmdList = context_.graphics->GetGraphicsDevice()->GetCommandList();
	auto* srvManager = context_.graphics->GetGraphicsDevice()->GetSrvManager();

	// テクスチャの初期化
	textureManager_ = std::make_unique<TextureManager>();
	textureManager_->Initialize(cmdList, srvManager);

	// モデルを管理するクラスを生成
	modelManager_ = std::make_unique<ModelManager>();
	modelManager_->Initialize(cmdList, textureManager_.get(), srvManager);

	// アニメーション
	animationManager_ = std::make_unique<AnimationManager>();

	// 音声
	AudioManager::GetInstance().Initialize();

	// パラメータファイルの読み込み
	gameParamEditor_ = std::make_unique<GameParamEditor>();
	DebugParameter::StaticInitialize(gameParamEditor_.get());

	// 全てのリソースをロードする
	LoadAllResources();
}

void ResourceSubsystem::LoadAllResources() {

	// 全てのパラメータファイルを読み込み
	gameParamEditor_->LoadFiles();

	// 画像データを全てロードする
	textureManager_->LoadAllTexture();

	// モデルデータを全てロードする
	modelManager_->RegisterGridPlaneModel("Grid", { 200.0f, 200.0f });
	modelManager_->RegisterRingModel("Ring", 32, 1.0f, 0.2f);
	modelManager_->LoadAllModel();

	// アニメーションデータをロード
	animationManager_->RegisterAnimation("Walk", "walk.gltf", "Resources/Models/Walk");
	animationManager_->RegisterAnimation("AnimatedCube", "AnimatedCube.gltf", "Resources/Models/AnimatedCube");

	// プレイヤーのアニメーションデータを読み込む
	animationManager_->RegisterAnimation("PlayerWalk", "PlayerWalk.gltf", "Resources/Animations/Player/PlayerWalk");
	animationManager_->RegisterAnimation("PlayerAirMove", "PlayerAirMove.gltf", "Resources/Animations/Player/PlayerAirMove");
	animationManager_->RegisterAnimation("PlayerRush", "PlayerRush.gltf", "Resources/Animations/Player/PlayerRush");
	animationManager_->RegisterAnimation("PlayerDownAttack", "DownAttack.gltf", "Resources/Animations/Player/PlayerDownAttack");

	// ボスのアニメーションデータを読み込む
	animationManager_->RegisterAnimation("BossBirdBaseMove", "BossBirdBaseMove.gltf", "Resources/Animations/Boss/BossBirdBaseMove");

	// 音声データを全てロードする
	AudioManager::GetInstance().LoadAllAudio();
}

void ResourceSubsystem::Finalize() {
	AudioManager::GetInstance().Finalize();
	textureManager_->Finalize();
}


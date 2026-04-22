#include "ResourceSubsystem.h"
#include "DebugParameter.h"
#include "GraphicsSubsystem.h"
using namespace GameEngine;

void ResourceSubsystem::Initialize() {
	auto* device = context_.graphics->GetGraphicsDevice()->GetDevice();
	auto* cmdList = context_.graphics->GetGraphicsDevice()->GetCommandList();
	auto* srvManager = context_.graphics->GetGraphicsDevice()->GetSrvManager();

	// テクスチャの初期化
	textureManager_ = std::make_unique<TextureManager>();
	textureManager_->Initialize(cmdList, srvManager);

	// モデルを管理するクラスを生成
	modelManager_ = std::make_unique<ModelManager>();
	modelManager_->Initialize(device, cmdList, textureManager_.get(), srvManager);

	// アニメーション
	animationManager_ = std::make_unique<AnimationManager>();

	// 音声
	AudioManager::GetInstance().Initialize();

	// パラメータファイルの読み込み
	gameParamEditor_ = std::make_unique<GameParamEditor>();
	// 全てのパラメータファイルを読み込み
	gameParamEditor_->LoadFiles();
	DebugParameter::StaticInitialize(gameParamEditor_.get());

	// 全てのリソースをロードする
	LoadAllResources();
}

void ResourceSubsystem::LoadAllResources() {
	// 画像データを全てロードする
	textureManager_->LoadAllTexture();

	// モデルデータを全てロードする
	modelManager_->RegisterGridPlaneModel("Grid", { 200.0f, 200.0f });
	modelManager_->LoadAllModel();

	// アニメーションデータをロード
	animationManager_->RegisterAnimation("Walk", "walk.gltf");

	// 音声データを全てロードする
	AudioManager::GetInstance().LoadAllAudio();
}

void ResourceSubsystem::Finalize() {
	AudioManager::GetInstance().Finalize();
	textureManager_->Finalize();
}


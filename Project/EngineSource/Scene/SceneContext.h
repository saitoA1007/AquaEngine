#pragma once
#include"GraphicsDevice.h"
#include"Input.h"
#include"TextureManager.h"
#include"AudioManager.h"
#include"InputCommand.h"
#include"ModelManager.h"
#include"DebugCamera.h"
#include"AnimationManager.h"

#include"RenderPass/RenderPassController.h"

// シーンで使用するエンジン機能
struct SceneContext {
	GameEngine::Input* input = nullptr; // 純粋な入力処理を取得
	GameEngine::InputCommand* inputCommand = nullptr; // 登録した入力処理を取得可能
	GameEngine::ModelManager* modelManager = nullptr; // モデルを取得可能
	GameEngine::TextureManager* textureManager = nullptr; // 画像を取得可能
	GameEngine::AudioManager* audioManager = nullptr; // 音を取得可能
	GameEngine::GraphicsDevice* graphicsDevice = nullptr; // DirectXのコア機能
	GameEngine::DebugCamera* debugCamera_ = nullptr; // デバック描画機能
	GameEngine::AnimationManager* animationManager = nullptr; // アニメーションデータを取得可能

	GameEngine::RenderPassController* renderPassController = nullptr; // 描画パスを管理する
};
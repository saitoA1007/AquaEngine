#include "Player.h"
#include <algorithm>
#include "GameParamEditor.h"
#include "EasingManager.h"
#include "MyMath.h"
#include "FPSCounter.h"
#include "Model.h"
using namespace GameEngine;

Player::Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model) {
	inputCommand_ = inputCommand;
	model_ = model;

	// ワールド行列を初期化
	worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{-2.0f,1.0f,0.0f} });

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Player");
	debugParame_->Register("speed", kMoveSpeed_);
}

void Player::Initialize() {
	
}

void Player::Update() {

	// プレイヤーの入力処理
	ProcessMoveInput(inputCommand_);

	// プレイヤーを移動範囲に制限
	worldTransform_.transform_.translate.x = std::clamp(worldTransform_.transform_.translate.x, -9.0f, 9.0f);
	worldTransform_.transform_.translate.z = std::clamp(worldTransform_.transform_.translate.z, -9.0f, 9.0f);

	// 行列の更新
	worldTransform_.UpdateTransformMatrix();
}

void Player::Draw() {

	// モデル描画
	renderQueue_->SubmitModel(model_, worldTransform_);
}

void Player::ProcessMoveInput(GameEngine::InputCommand* inputCommand) {

	// プレイヤーの移動操作
	if (inputCommand->IsCommandAcitve("MoveUp")) {
		worldTransform_.transform_.translate.z += kMoveSpeed_;
	}

	if (inputCommand->IsCommandAcitve("MoveDown")) {
		worldTransform_.transform_.translate.z -= kMoveSpeed_;
	}

	if (inputCommand->IsCommandAcitve("MoveLeft")) {
		worldTransform_.transform_.translate.x -= kMoveSpeed_;
	}

	if (inputCommand->IsCommandAcitve("MoveRight")) {
		worldTransform_.transform_.translate.x += kMoveSpeed_;
	}

	// ジャンプ操作
	if (inputCommand->IsCommandAcitve("Jump")) {
		
	}
}

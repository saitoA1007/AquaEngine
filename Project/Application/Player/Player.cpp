#include"Player.h"
#include<algorithm>
#include"GameParamEditor.h"
#include"EasingManager.h"
#include"MyMath.h"
#include"FPSCounter.h"
using namespace GameEngine;

void Player::Initialize() {

	// ワールド行列を初期化
	worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{-2.0f,1.0f,0.0f} });

#ifdef _DEBUG
	//===========================================================
	// 
	// 現在、saveを押していないので、Playerのjsonファイルは存在していません
	// 
	//===========================================================

	// 値を登録する
	RegisterBebugParam();
#else
	// jsonファイルが作られていない状態で値の適応をおこなうとリリース版でバクります
	// 値を適応させる
	ApplyDebugParam();
#endif
}

void Player::Update(GameEngine::InputCommand* inputCommand) {
#ifdef _DEBUG
	// 値を適応
	ApplyDebugParam();
#endif

	// プレイヤーの入力処理
	ProcessMoveInput(inputCommand);

	// プレイヤーのジャンプ処理
	JumpUpdate();

	// プレイヤーを移動範囲に制限
	worldTransform_.transform_.translate.x = std::clamp(worldTransform_.transform_.translate.x,-9.0f,9.0f);
	worldTransform_.transform_.translate.z = std::clamp(worldTransform_.transform_.translate.z, -9.0f, 9.0f);

	// 行列の更新
	worldTransform_.UpdateTransformMatrix();
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
		if (isJump_) { return; }
		isJump_ = true;
		jumpTimer_ = 0.0f;
	}
}

void Player::JumpUpdate() {
	// フラグが立っていなければ早期リターン
	if (!isJump_) { return; }

	jumpTimer_ += 1.0f / (FpsCounter::maxFrameCount * kJumpMaxTime_);

	// ジャンプ処理
	if (jumpTimer_ <= 0.5f) {
		// ジャンプの上昇
		float jumpUpTimer_ = jumpTimer_ / 0.5f;
		worldTransform_.transform_.translate.y = Lerp(1.0f, kJumpHeight_, EaseOut(jumpUpTimer_));
	} else {
		// ジャンプの下降
		float jumpDownTimer_ = (jumpTimer_ - 0.5f) / 0.5f;
		worldTransform_.transform_.translate.y = Lerp(kJumpHeight_, 1.0f, EaseIn(jumpDownTimer_));
	}

	// 時間がたったらフラグをfalse
	if (jumpTimer_ >= 1.0f) {
		isJump_ = false;
	}
}

void Player::RegisterBebugParam() {
	// 値の登録
	GameParamEditor::GetInstance()->AddItem("Player", "JumpMaxHeight", kJumpHeight_);
	GameParamEditor::GetInstance()->AddItem("Player", "JumpMaxTime", kJumpMaxTime_);
	GameParamEditor::GetInstance()->AddItem("Player", "MoveSpeed", kMoveSpeed_);
}

void Player::ApplyDebugParam() {
	// 値の適応
	kJumpHeight_ = GameParamEditor::GetInstance()->GetValue<float>("Player", "JumpMaxHeight");
	kJumpMaxTime_ = GameParamEditor::GetInstance()->GetValue<float>("Player", "JumpMaxTime");
	kMoveSpeed_ = GameParamEditor::GetInstance()->GetValue<float>("Player", "MoveSpeed");
}
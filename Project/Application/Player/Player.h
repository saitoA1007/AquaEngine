#pragma once
#include"WorldTransform.h"
#include"InputCommand.h"

class Player {
public:

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="model"></param>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	/// <param name="inputCommand"></param>
	void Update(GameEngine::InputCommand* inputCommand);

	/// <summary>
	/// ワールド行列を取得
	/// </summary>
	/// <returns></returns>
	GameEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

	/// <summary>
	/// プレイヤーの位置を取得
	/// </summary>
	/// <returns></returns>
	Vector3 GetPlayerPos() { return worldTransform_.GetWorldPosition(); }

private:

	// ジャンプの高さ
	float kJumpHeight_ = 4.0f;
	// ジャンプする時間
	float kJumpMaxTime_ = 0.65f;

	// 移動速度
	float kMoveSpeed_ = 0.2f;

private:

	// ワールド行列
	GameEngine::WorldTransform worldTransform_;

	// ジャンプフラグ
	bool isJump_ = false;
	
	// ジャンプタイマー
	float jumpTimer_ = 0.0f;

private:

	/// <summary>
	/// プレイヤーの入力処理
	/// </summary>
	/// <param name="inputCommand"></param>
	void ProcessMoveInput(GameEngine::InputCommand* inputCommand);

	/// <summary>
	/// ジャンプする処理
	/// </summary>
	void JumpUpdate();

	/// <summary>
	/// 値を登録する
	/// </summary>
	void RegisterBebugParam();

	/// <summary>
	/// 値を適応する
	/// </summary>
	void ApplyDebugParam();
};
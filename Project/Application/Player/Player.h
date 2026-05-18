#pragma once
#include "IGameObject.h"
#include "WorldTransform.h"
#include "InputCommand.h"
#include "DebugParameter.h"

class Player : public GameEngine::IGameObject {
public:
	Player(GameEngine::InputCommand* inputCommand, GameEngine::Model* model);
	~Player() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;


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

	// 移動速度
	float kMoveSpeed_ = 0.2f;

private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 入力機能
	GameEngine::InputCommand* inputCommand_ = nullptr;

	// モデル
	GameEngine::Model* model_ = nullptr;

	// ワールド行列
	GameEngine::WorldTransform worldTransform_;

private:

	/// <summary>
	/// プレイヤーの入力処理
	/// </summary>
	/// <param name="inputCommand"></param>
	void ProcessMoveInput(GameEngine::InputCommand* inputCommand);
};
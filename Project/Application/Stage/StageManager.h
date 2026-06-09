#pragma once
#include <vector>
#include "IGameObject.h"
#include "Model.h"
#include "GameObjectManager.h"
#include "Wall.h"
#include "DebugParameter.h"

class StageManager : public GameEngine::IGameObject {
public:
	StageManager(GameEngine::GameObjectManager* objectManager, GameEngine::Model* model);
	~StageManager() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

private:
	GameEngine::GameObjectManager* objectManager_ = nullptr;
	GameEngine::Model* model_ = nullptr;

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 生成位置
	Vector3 centerPos_ = { 0.0f,1.0f,0.0f };
	// 辺の数
	uint32_t maxSideNumber_ = 8;
	// 半径
	float radius_ = 12.0f;

	/// 壁のデータ
	// 最大hp
	int32_t maxHp_ = 3;
	// 壁の奥行
	float wallDepth_ = 0.5f;
	// 壁の縦幅
	float wallHeight_ = 2.0f;
	// 壁の横幅の余剰分
	float offsetWallWidth_ = -3.0f;
	// 復活するまでの時間
	float respawnTime_ = 3.0f;

	// 壁のデータ
	std::vector<Wall*> walls_;

private:

	// ステージを生成する
	void GenerateWalls();

	// 値を登録
	void RegisterParameter();

};
#pragma once
#include "IGameObject.h"
#include "GameObjectManager.h"
#include "Model.h"

class BossRangedAttackManager : public GameEngine::IGameObject {
public:
	BossRangedAttackManager(GameEngine::GameObjectManager* objectManager, GameEngine::Model* iceFallModel);
	~BossRangedAttackManager() = default;

	// 初期化
	//void Initialize() override;
	//
	//// 更新処理
	//void Update() override;
	//
	//// 描画処理
	//void Draw() override;
	
public:

	/// <summary>
	/// 氷柱攻撃
	/// </summary>
	/// <param name="basePos">基準点</param>
	/// <param name="rangeRadius">生成範囲</param>
	/// <param name="minDistance">離れる範囲</param>
	/// <param name="targetCount">出す数</param>
	/// <param name="maxIter">最大試行回数</param>
	void StartIceFall(float rangeRadius, float minDistance, int targetCount, int maxIter);

	/// <summary>
	/// 風攻撃
	/// </summary>
	/// <param name="pos">位置</param>
	/// <param name="startDir">最初の方向</param>
	/// <param name="endDir">最後の方向</param>
	void StartWind(Vector3 pos, Vector3 startDir, Vector3 endDir);

private:
	GameEngine::GameObjectManager* objectManager_ = nullptr;

	GameEngine::Model* iceFallModel_ = nullptr;
};
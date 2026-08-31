#pragma once
#include "IGameObject.h"
#include "GameObjectManager.h"
#include "Model.h"
#include "Application/Enemy/RangedAttack/WindAttack.h"

namespace GameEngine {
	class TextureManager;
	class Camera;
	class ParticleBehavior;
}

class BossRangedAttackManager : public GameEngine::IGameObject {
public:
	BossRangedAttackManager(GameEngine::GameObjectManager* objectManager, GameEngine::Model* iceFallModel, GameEngine::Model* iceFallFractureModel,
		GameEngine::TextureManager* textureManager, GameEngine::Model* windModel, GameEngine::Camera* camera);
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
	/// <param name="rangeRadius">生成範囲</param>
	/// <param name="minDistance">最小の離れる距離</param>
	/// <param name="iceFallNum">生成する数</param>
	/// <param name="iceFallMaxNum">最大の数</param>
	/// <param name="maxIter">試行回数</param>
	void StartIceFall(float rangeRadius, float minDistance, int iceFallNum, int iceFallMaxNum, int maxIter);

	/// <summary>
	/// 風攻撃
	/// </summary>
	/// <param name="pos">位置</param>
	/// <param name="startDir">最初の方向</param>
	/// <param name="endDir">最後の方向</param>
	void StartWind(Vector3 pos, Vector3 startDir, Vector3 endDir, float maxTime);

private:
	GameEngine::GameObjectManager* objectManager_ = nullptr;

	GameEngine::Model* iceFallModel_ = nullptr;
	GameEngine::Model* iceFallFractureModel_ = nullptr;

	// 氷柱の現在の数
	int32_t currentIceFallNum_ = 0;

	// 風の攻撃演出
	WindAttack* windAttack_ = nullptr;
};
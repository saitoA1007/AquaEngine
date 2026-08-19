#include "BossRangedAttackManager.h"
#include <numbers>
#include "RandomGenerator.h"
#include "Application/Enemy/RangedAttack/IceFall.h"
#include "ParticleBehavior.h"
using namespace GameEngine;

BossRangedAttackManager::BossRangedAttackManager(GameEngine::GameObjectManager* objectManager, GameEngine::Model* iceFallModel, GameEngine::Model* iceFallFractureModel,
	GameEngine::TextureManager* textureManager, GameEngine::Model* windModel, GameEngine::Camera* camera) {

	objectManager_ = objectManager;

	iceFallModel_ = iceFallModel;

    iceFallFractureModel_ = iceFallFractureModel;

	// 風攻撃の軌跡パーティクルを生成
	auto* windParticle = objectManager_->AddObject<ParticleBehavior>("EnemyWindAttackParticle", 128, textureManager, windModel, camera);
	windParticle->SetIsLoop(false);

    // 風エフェクトを生成
    windAttack_ = objectManager_->AddObject<WindAttack>(iceFallModel_, windParticle);
}

void BossRangedAttackManager::StartIceFall(float rangeRadius, float minDistance, int iceFallNum, int iceFallMaxNum, int maxIter) {
    if (currentIceFallNum_ > iceFallMaxNum) { return; }

    std::vector<Vector2> points;
    int attempts = 0;

    // 生成してた数
    int count = 0;

    while (count < iceFallNum && attempts < maxIter) {
        attempts++;

        // 大きな円の中にランダムな点を生成
        float r = rangeRadius * std::sqrt(RandomGenerator::Get(0.0f, 1.0f));
        float theta = RandomGenerator::Get(0.0f, std::numbers::pi_v<float> *2.0f);

        Vector2 candidate;
        candidate.x = r * std::cos(theta);
        candidate.y = r * std::sin(theta);

        // 既存の点との距離をチェック
        bool isValid = true;
        float minDistSq = minDistance * minDistance;

        for (const auto& p : points) {
            // 近すぎる点があれば即座に却下
            if (Vector2::GetDistance(candidate, p) < minDistSq) {
                isValid = false;
                break; 
            }
        }

        // 条件を満たせば採用
        if (isValid) {
            count++;
            points.push_back(candidate);
        }
    }

    // 求めた位置から氷を生成する
    for (size_t i = 0; i < points.size(); ++i) {
        objectManager_->AddObject<IceFall>(iceFallModel_, iceFallFractureModel_, Vector3(points[i].x, 0.0f, points[i].y), currentIceFallNum_);
    }
}

void BossRangedAttackManager::StartWind(Vector3 pos, Vector3 startDir, Vector3 endDir, float maxTime) {
    // 風の演出を開始
    windAttack_->Start(pos, startDir, endDir, maxTime);
}
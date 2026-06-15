#include "BossRangedAttackManager.h"
#include <numbers>
#include "RandomGenerator.h"
#include "Application/Enemy/RangedAttack/IceFall.h"
#include "Application/Enemy/RangedAttack/WindAttack.h"
using namespace GameEngine;

BossRangedAttackManager::BossRangedAttackManager(GameEngine::GameObjectManager* objectManager, GameEngine::Model* iceFallModel) {

	objectManager_ = objectManager;

	iceFallModel_ = iceFallModel;

}

void BossRangedAttackManager::StartIceFall(float rangeRadius, float minDistance, int targetCount, int maxIter) {
    std::vector<Vector2> points;
    int attempts = 0;

    // 生成してた数
    int count = 0;

    while (count < targetCount && attempts < maxIter) {
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
        objectManager_->AddObject<IceFall>(iceFallModel_, Vector3(points[i].x, 0.0f, points[i].y));
    }
}

void BossRangedAttackManager::StartWind(Vector3 pos, Vector3 startDir, Vector3 endDir) {
    // 風を生成
    objectManager_->AddObject<WindAttack>(iceFallModel_, pos, startDir, endDir);
}
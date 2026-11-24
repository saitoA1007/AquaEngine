#include "pch.h"
#include "BossRangedAttackManager.h"
#include <numbers>
#include <algorithm>
#include "RandomGenerator.h"
#include "Application/Enemy/RangedAttack/IceFall.h"
#include "Application/Enemy/RangedAttack/HealingHeart.h"
#include "Application/Player/Player.h"
#include "ParticleBehavior.h"
using namespace GameEngine;

BossRangedAttackManager::BossRangedAttackManager(GameEngine::GameObjectManager* objectManager, GameEngine::Model* iceFallModel, GameEngine::Model* iceFallFractureModel,
	GameEngine::TextureManager* textureManager, GameEngine::Model* windModel, GameEngine::Model* rushModel, GameEngine::Model* heartModel) {

	objectManager_ = objectManager;

	iceFallModel_ = iceFallModel;

    iceFallFractureModel_ = iceFallFractureModel;

	// 風攻撃の軌跡パーティクルを生成
	auto* windParticle = objectManager_->AddObject<ParticleBehavior>("EnemyWindAttackParticle", 256, textureManager, windModel);
	windParticle->SetIsLoop(false);

    // 突進パーティクルを生成
    rushParticle_ = objectManager_->AddObject<ParticleBehavior>("BossRushEffect", 32, textureManager, rushModel);
    rushParticle_->SetIsLoop(false);

    // 風エフェクトを生成
    windAttack_ = objectManager_->AddObject<WindAttack>(iceFallModel_, windParticle);

    // 氷柱をあらかじめ確保しておく
    iceFalls_.reserve(6);
    hearts_.reserve(6);
    for (int i = 0; i < 6; ++i) {
        iceFalls_.push_back(objectManager_->AddObject<IceFall>(iceFallModel_, iceFallFractureModel_, Vector3(0.0f,0.0f,0.0f), currentIceFallNum_));

        // 氷柱の中に入れるハート
        hearts_.push_back(objectManager_->AddObject<HealingHeart>(heartModel));
        iceFalls_.back()->SetHeart(hearts_.back());
    }

    // パラメータ機能
    debugParame_ = std::make_unique<DebugParameter>("BossRangedAttack");
    int index = 0;
    debugParame_->Register("HeartChance", heartChance_, index++, "Heart");
    debugParame_->Register("HeartMaxNumPerAttack", heartMaxNumPerAttack_, index++, "Heart");
    debugParame_->Apply();
}

void BossRangedAttackManager::Update() {
    debugParame_->ApplyIfDirty();
}

bool BossRangedAttackManager::ShouldPutHeart(int32_t putCount) const {
    // プレイヤーのHpが減っていなければ入れない
    if (player_ == nullptr) { return false; }
    if (player_->GetCurrentHp() >= player_->GetMaxHp()) { return false; }

    // 1回の攻撃で入れる数の上限
    if (putCount >= heartMaxNumPerAttack_) { return false; }

    // 確率で入れる
    return RandomGenerator::Get(0.0f, 1.0f) < heartChance_;
}

void BossRangedAttackManager::StartIceFall(float rangeRadius, float minDistance, int iceFallNum, int iceFallMaxNum, int maxIter) {
    // 上限に達していたら生成しない
    if (currentIceFallNum_ >= iceFallMaxNum) { return; }

    // 生成数を残り枠（上限 − 現在数）以下にする
    iceFallNum = std::min(iceFallNum, iceFallMaxNum - currentIceFallNum_);

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
            if (Vector2::DistanceSquared(candidate, p) < minDistSq) {
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

    // 求めた位置に、待機中の氷柱を出現させる
    size_t poolIndex = 0;
    int32_t heartCount = 0;
    for (const Vector2& point : points) {

        // 待機中の氷柱を探す
        while (poolIndex < iceFalls_.size() && iceFalls_[poolIndex]->IsActive()) {
            ++poolIndex;
        }
        // 全て使用中
        if (poolIndex >= iceFalls_.size()) { break; }

        // 確率でハートを入れる
        bool withHeart = iceFalls_[poolIndex]->CanPutHeart() && ShouldPutHeart(heartCount);
        if (withHeart) {
            heartCount++;
        }
        iceFalls_[poolIndex]->Start(Vector3(point.x, 0.0f, point.y), withHeart);
        poolIndex++;
    }
}

void BossRangedAttackManager::StartWind(Vector3 pos, Vector3 startDir, Vector3 endDir, float maxTime) {
    // 風の演出を開始
    windAttack_->Start(pos, startDir, endDir, maxTime);
}

void BossRangedAttackManager::Reset() {
    // 風攻撃を止める
    windAttack_->Stop();

    // 突進パーティクルを止める
    rushParticle_->SetIsLoop(false);
    rushParticle_->Clear();

    // 氷柱を全て待機状態に戻す
    for (IceFall* iceFall : iceFalls_) {
        iceFall->Stop();
    }
}

void BossRangedAttackManager::SetRush(Vector3 pos, Vector3 dir, bool isActive) {
    rushParticle_->SetIsLoop(isActive);
    rushParticle_->SetEmitterPos(pos);
    rushParticle_->SetDirection(dir);
}
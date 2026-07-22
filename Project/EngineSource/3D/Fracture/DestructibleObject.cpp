#include "DestructibleObject.h"
#include "RandomGenerator.h"
#include "FPSCounter.h"
using namespace GameEngine;

DestructibleObject::DestructibleObject(Model* model, uint32_t colliderId, uint32_t colliderAttribute) {

	model_ = model;

    collider_.SetCollisionAttribute(colliderAttribute);
    collider_.SetCollisionMask(~colliderAttribute);

    UserData userData;
    userData.typeID = colliderId;
    userData.object = this;
    collider_.SetUserData(userData);

    // 接触した瞬間だけ破壊処理を呼ぶ
    collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) {
        OnCollisionEnter(result);
    });

    int i = 0;
    for (auto& [groupName, chunks] : model_->GetFractureChunks()) {
        if (i != 0) { continue; }
        PackedGeometryBuffer* buffer = model_->GetFractureBuffers().at(groupName).get();

        std::vector<uint32_t> chunkIds;
        const auto& fractureChunks = model_->GetFractureChunks().begin()->second;
        for (const auto& chunk : fractureChunks) {
            chunkIds.push_back(chunk.info.chunkId);
        }

        fractureInstance_.Initialize(chunkIds, *buffer);
        i++;
    }
}

void DestructibleObject::Initialize() {

}

void DestructibleObject::Update() {

    // 破片が動くかテスト
    auto& transforms = fractureInstance_.GetTransformDatas();
    for (size_t i = 0; i < transforms.size(); ++i) {
        // ここで物理演算や簡単な移動計算を行う
        transforms[i].velocity += RandomGenerator::GetVector3(-1.0f, 1.0f) * 0.05f;
        transforms[i].transform.translate += transforms[i].velocity * FpsCounter::gameDeltaTime;
    }
    fractureInstance_.Update();
}

void DestructibleObject::Draw() {
    // 破片を描画
    renderQueue_->SubmitFracture(model_, fractureInstance_);
}

void DestructibleObject::OnCollisionEnter(const GameEngine::CollisionResult& result) {

    ApplyDamage(result.contactPosition, result.penetrationDepth * 2.0f + 0.5f);
}

void DestructibleObject::ApplyDamage(const Vector3& impactPos, float damageRadius) {

    // 一番近いチャンクをシードとして特定
    // SelectDetachedChunksでチャンク範囲を選ぶ
    // シードチャンクだけExtractChunk + ApplyRuntimeCut
    // 静的メッシュ側から対象チャンクを非表示化

}
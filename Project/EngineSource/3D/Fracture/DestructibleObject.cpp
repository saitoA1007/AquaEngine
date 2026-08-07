#define NOMINMAX
#include "DestructibleObject.h"
#include "FPSCounter.h"
using namespace GameEngine;

DestructibleObject::DestructibleObject(Model* model, uint32_t colliderId, uint32_t colliderAttribute) {
	model_ = model;
	colliderId_ = colliderId;
	colliderAttribute_ = colliderAttribute;
}

void DestructibleObject::Initialize() {

	// 当たり判定の設定
	collider_.SetCollisionAttribute(colliderAttribute_);
	collider_.SetCollisionMask(~colliderAttribute_);
	collider_.SetSize(colliderSize_);
	collider_.SetWorldPosition(worldTransform_.GetWorldPosition());
	collider_.SetAnchorPoint(Vector3(0.5f, 0.5f, 0.5f));

	UserData userData;
	userData.typeID = colliderId_;
	userData.object = this;
	collider_.SetUserData(userData);

	// コールバック関数を登録
	collider_.SetOnCollisionEnterCallback([this](const CollisionResult& result) {
		OnCollisionEnter(result);
		});

	// 氷のマテリアルを設定する
	for (auto& [groupName, chunks] : model_->GetFractureChunks()) {
		PackedGeometryBuffer* buffer = model_->GetFractureBuffers().at(groupName).get();
		buffer->SetBufferMaterial(iceMaterial_.GetMaterialSrvIndex());
	}

	// 破砕状態を初期化（全チャンク無傷）
	damageController_.Initialize(model_);

	// パラメータ機能
	debugParameter_ = std::make_unique<DebugParameter>("DestructibleObject");
	debugParameter_->Register("ColliderSize", colliderSize_);
	debugParameter_->Register("TestDamageAmount", testDamageAmount_);
	debugParameter_->Register("TestCraterRadius", testCraterRadius_);
	debugParameter_->Register("TestPlaneCount", testPlaneCount_);
	debugParameter_->Apply();
}

void DestructibleObject::Update() {

	debugParameter_->ApplyIfDirty();

	// 更新
	worldTransform_.UpdateTransformMatrix();
	collider_.SetWorldPosition(worldTransform_.GetWorldPosition());
	collider_.SetSize(colliderSize_);

	damageController_.Update(FpsCounter::gameDeltaTime);
}

void DestructibleObject::Draw() {

	FractureBreakState& breakState = damageController_.GetBreakState();

	// 事前分割された静的な破片はレイトレで描画
	if (breakState.HasIntact()) {
		renderQueue_->SubmitRaytracingFracture(model_, breakState.Intact(), worldTransform_);
	}
	if (breakState.HasMacroDebris()) {
		renderQueue_->SubmitRaytracingFracture(model_, breakState.MacroDebris(), worldTransform_);
	}

	// ランタイムでカットされた破片はラスタライズで描画
	if (breakState.HasMicroDebris()) {
		const auto& fractureChunks = model_->GetFractureChunks();
		if (!fractureChunks.empty()) {
			const auto& chunks = fractureChunks.begin()->second;
			Material* drawMaterial = model_->GetMaterial(chunks.front().materialName);
			renderQueue_->SubmitRuntimeCutFragments(breakState.MicroDebris(), &drawMaterial->GetMaterialBuffer());
		}
	}
}

void DestructibleObject::OnCollisionEnter(const GameEngine::CollisionResult& result) {
	damageController_.ApplyChipDamage(result.contactPosition, testDamageAmount_, testCraterRadius_, testPlaneCount_);
}

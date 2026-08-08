#define NOMINMAX
#include "DestructibleObject.h"
#include "FPSCounter.h"
using namespace GameEngine;

DestructibleObject::DestructibleObject(std::string name, Model* model, uint32_t colliderId, uint32_t colliderAttribute) {
	name_ = name;
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

	// 破砕状態を初期化
	damageController_.Initialize(model_);

	std::string subGroup = "";

	// パラメータ機能
	debugParameter_ = std::make_unique<DebugParameter>(name_);
	debugParameter_->Register("ColliderSize", colliderSize_);
	debugParameter_->Register("TestDamageAmount", testDamageAmount_);
	debugParameter_->Register("TestCraterRadius", testCraterRadius_);
	debugParameter_->Register("TestPlaneCount", testPlaneCount_);

	debugParameter_->Register("BreakThreshold", damageController_.kBreakThreshold_);
	debugParameter_->Register("MaxCrackOffset", damageController_.kMaxCrackOffset_);
	debugParameter_->Register("MaxCrackRotate", damageController_.kMaxCrackRotate_);
	debugParameter_->Register("NeighborCrackFactor", damageController_.kNeighborCrackFactor_);
	// バネ物理
	subGroup = "Physics";
	debugParameter_->Register("CrackSpringStiffness", damageController_.kCrackSpringStiffness_,0, subGroup);
	debugParameter_->Register("CrackDamping", damageController_.kCrackDamping_, 0, subGroup);
	debugParameter_->Register("CrackAngularSpringStiffness", damageController_.kCrackAngularSpringStiffness_, 0, subGroup);
	debugParameter_->Register("CrackAngularDamping", damageController_.kCrackAngularDamping_, 0, subGroup);
	debugParameter_->Register("CrackImpulseStrength", damageController_.kCrackImpulseStrength_, 0, subGroup);

	debugParameter_->Register("DentInwardBiasRatio", damageController_.kDentInwardBiasRatio_);
	debugParameter_->Register("MinDentRatio", damageController_.kMinDentRatio_);
	debugParameter_->Register("MaxDentRadiusToChunkRatio", damageController_.kMaxDentRadiusToChunkRatio_);

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
	// マクロ破片は破壊イベントごとに積み上がったバッチ全てを描画する
	for (auto& batch : breakState.MacroDebrisBatches()) {
		renderQueue_->SubmitRaytracingFracture(model_, *batch, worldTransform_);
	}

	// ランタイムでカットされた破片はラスタライズで描画
	if (breakState.HasMicroDebris() || breakState.HasDentedChunks()) {
		const auto& fractureChunks = model_->GetFractureChunks();
		if (!fractureChunks.empty()) {
			const auto& chunks = fractureChunks.begin()->second;
			Material* drawMaterial = model_->GetMaterial(chunks.front().materialName);

			// 積み上がったマイクロ破片バッチ全てを描画する
			for (auto& batch : breakState.MicroDebrisBatches()) {
				//renderQueue_->SubmitRuntimeCutFragments(*batch, &drawMaterial->GetMaterialBuffer());
				renderQueue_->SubmitRuntimeCutIceFragments(*batch, &iceMaterial_.GetMaterialBuffer());
			}

			// 付着したまま動的に凹んでいるチャンクを描画する
			for (auto& [chunkId, instance] : breakState.DentedChunks()) {
				//renderQueue_->SubmitRuntimeCutFragments(*instance, &drawMaterial->GetMaterialBuffer());
				renderQueue_->SubmitRuntimeCutIceFragments(*instance, &iceMaterial_.GetMaterialBuffer());
			}
		}
	}
}

void DestructibleObject::OnCollisionEnter(const GameEngine::CollisionResult& result) {
	damageController_.ApplyChipDamage(result.contactPosition, testDamageAmount_, testCraterRadius_, testPlaneCount_,
		result.contactNormal, result.penetrationDepth);
}

#include "pch.h"
#include "Floor.h"
#include "Application/CollisionConfig.h"
#include "FPSCounter.h"
#include "ImGuiManager.h"
using namespace GameEngine;

Floor::Floor(GameEngine::Model* model, GameEngine::Model* fractureModel, uint32_t iceNormalGH, uint32_t iceHeightGH, uint32_t terrainGH, uint32_t terrainNormalGH)
	: iceModelComponent_(model), terrainModelComponent_(model),
	destructObject_("FloorFracture", fractureModel, static_cast<uint32_t>(CollisionTypeID::kGround), kCollisionAttributeTerrain) {

	// 氷の法線マップ
	iceMaterial_.materialData_->dissolveThreshold = 0.0f;
	iceMaterial_.materialData_->heightScale = 0.002f;
	iceMaterial_.materialData_->heightTextureHandle = iceHeightGH;
	iceMaterial_.materialData_->normalTextureHandle = iceNormalGH;
	// 地面
	terrainModelComponent_.materialData_->textureHandle = terrainGH;
	terrainModelComponent_.materialData_->normalTextureHandle = terrainNormalGH;
	terrainModelComponent_.materialData_->enableLighting = false;
	terrainModelComponent_.materialData_->metallic = 0.01f;
	terrainModelComponent_.materialData_->roughness = 0.9f;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>("Floor");
	debugParame_->RegisterWorld("world", iceModelComponent_.worldTransform_);
	debugParame_->Register("ColliderSize", colliderSize_);
	debugParame_->Register("ColliderAnchor", colliderAnchor_);
	std::string subGroup = "IceMaterial";
	int index = 0;
	debugParame_->Register("bubbleScale", iceMaterial_.materialData_->bubbleScale, index++,subGroup);
	debugParame_->Register("bubbleMaxDepth", iceMaterial_.materialData_->bubbleMaxDepth, index++,subGroup);
	debugParame_->Register("bubbleDensity", iceMaterial_.materialData_->bubbleDensity, index++,subGroup);
	debugParame_->Register("bubbleJitter", iceMaterial_.materialData_->bubbleJitter, index++,subGroup);
	debugParame_->Register("bubbleHighlight", iceMaterial_.materialData_->bubbleHighlight, index++,subGroup);

	debugParame_->Register("rimColor", iceMaterial_.materialData_->rimColor, index++,subGroup);
	debugParame_->Register("rimIntensity", iceMaterial_.materialData_->rimIntensity, index++,subGroup);
	debugParame_->Register("rimPower", iceMaterial_.materialData_->rimPower, index++,subGroup);
	subGroup = "Fracture";
	index = 0;
	debugParame_->Register("FractureScale", fractureScale_, index++, subGroup);
	debugParame_->Register("OuterIceOffsetY", outerIceOffsetY_, index++, subGroup);
	debugParame_->Register("BreakDamage", breakDamage_, index++, subGroup);
	debugParame_->Register("BreakRadius", breakRadius_, index++, subGroup);
	debugParame_->Register("RespawnTime", respawnTime_, index++, subGroup);
	debugParame_->Apply();

	// 当たり判定
	collider_.SetWorldPosition(iceModelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.SetAnchorPoint(colliderAnchor_);
	collider_.SetCollisionAttribute(kCollisionAttributeTerrain);
	collider_.SetCollisionMask(~kCollisionAttributeTerrain);
	// データを登録
	UserData userData;
	userData.typeID = static_cast<uint32_t>(CollisionTypeID::kGround);
	userData.object = this;
	collider_.SetUserData(userData);
	// コールバック関数に登録する
	collider_.SetOnCollisionCallback([this](const CollisionResult& result) {
		this->OnCollisionEnter(result);
	});

	// 破片側の当たり判定は使わず、床の当たり判定で処理する
	destructObject_.SetIsColliderActive(false);
	// 床の破片は全て底面まで届いているため、アンカーを使わずに切り離せるようにする
	destructObject_.SetUseAnchor(false);
	destructObject_.iceMaterial_.GetMaterialData()->dissolveThreshold = 0.0f;

	// 参照するマテリアルを変更
	iceModelComponent_.SetBufferMaterial(0, iceMaterial_.GetMaterialSrvIndex());
	iceModelComponent_.SetHitGroup(1);

	Update();
}

void Floor::Initialize() {
	collider_.SetWorldPosition(iceModelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.SetAnchorPoint(colliderAnchor_);
	iceModelComponent_.Update();

	// 上層部を無傷の状態に戻す
	destructObject_.ResetToIntact();
	isBroken_ = false;
	respawnTimer_ = 0.0f;
}

void Floor::Update() {
	debugParame_->ApplyIfDirty();

	// 上層部の上面を氷の床面に合わせる(破壊モデルのYは-1~1)
	const Vector3 baseTranslate = iceModelComponent_.worldTransform_.transform_.translate;
	destructObject_.worldTransform_.transform_.scale = fractureScale_;
	destructObject_.worldTransform_.transform_.translate = baseTranslate;
	destructObject_.worldTransform_.transform_.translate.y -= fractureScale_.y;
	destructObject_.Update();

	// 壊れてから一定時間で元に戻す
	if (isBroken_ && !destructObject_.IsReassembling()) {
		respawnTimer_ += FpsCounter::gameDeltaTime / respawnTime_;
		if (respawnTimer_ >= 1.0f) {
			isBroken_ = false;
			respawnTimer_ = 0.0f;
			destructObject_.Reassemble();
		}
	}

	// 外側の氷の平面は上層部と重ならないように少し下げる(保存される値は変えない)
	iceModelComponent_.worldTransform_.transform_.translate.y -= outerIceOffsetY_;

	// 氷より少し下に地面を配置する
	terrainModelComponent_.worldTransform_.transform_.scale = iceModelComponent_.worldTransform_.transform_.scale;
	terrainModelComponent_.worldTransform_.transform_.translate = iceModelComponent_.worldTransform_.transform_.translate;
	terrainModelComponent_.worldTransform_.transform_.translate.y -= 0.2f;

	// 更新
	iceModelComponent_.Update();
	terrainModelComponent_.Update();
	iceModelComponent_.worldTransform_.transform_.translate = baseTranslate;

	collider_.SetWorldPosition(iceModelComponent_.worldTransform_.transform_.translate);
	collider_.SetSize(colliderSize_);
	collider_.SetAnchorPoint(colliderAnchor_);
}

void Floor::Draw() {
	// 上層部を描画
	destructObject_.Draw();

	// 外側の氷を描画
	iceModelComponent_.DrawRaytracing(renderQueue_);

	// 地面を描画
	terrainModelComponent_.DrawRaytracing(renderQueue_);
}

void Floor::Break(const Vector3& worldPos) {
	// 上から叩きつけた方向でダメージを与える
	destructObject_.ApplyDamage(worldPos, Vector3(0.0f, -1.0f, 0.0f), breakDamage_, breakRadius_);
	isBroken_ = true;
	respawnTimer_ = 0.0f;
}

void Floor::OnCollisionEnter([[maybe_unused]] const GameEngine::CollisionResult& result) {

}
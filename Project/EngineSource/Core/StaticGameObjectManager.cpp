#include "StaticGameObjectManager.h"
#include "CollisionUtils.h"
#include "MyMath.h"
using namespace GameEngine;

void StaticGameObjectManager::Initialize(GameObjectManager* objectManager, ModelManager* modelManager) {
	objectManager_ = objectManager;
	modelManager_ = modelManager;

}

uint32_t StaticGameObjectManager::AddObject(std::string objecctName, std::string modelName) {

	// モデルを取得
	auto* model = modelManager_->GetNameByModel(modelName);

	// オブジェクトを追加
	auto* object = objectManager_->AddObject<StaticGameObject>(objecctName, model);

	// idを取得
	uint32_t id = currentIndex_++;
	
	// オブジェクトを追加
	objects_[id] = object;

	return id;
}

void StaticGameObjectManager::ReleaseObject(uint32_t id) {
	auto it = objects_.find(id);
	assert(it != objects_.end() && "StaticObject not found");
	StaticGameObject* object = it->second;
	// オブジェクトの無効化
	object->SetActive(false);
}

void StaticGameObjectManager::RestoreObject(uint32_t id) {
	auto it = objects_.find(id);
	assert(it != objects_.end() && "StaticObject not found");
	StaticGameObject* object = it->second;
	// オブジェクトの有効化
	object->SetActive(true);
}

StaticGameObject* StaticGameObjectManager::GetStaticObject(uint32_t id) {
	auto it = objects_.find(id);
	assert(it != objects_.end() && "StaticObject not found");
	if (!it->second->IsActive()) {
		// 論理削除済みのオブジェクトはnullを返す
		return nullptr;
	}
	StaticGameObject* object = it->second;
	return object;
}

int32_t StaticGameObjectManager::SelectObject(Vector2 mousePos, const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix, const Vector3& cameraPosition, float width, float height) {

	Vector3 rayOrigin = cameraPosition;
	Vector3 rayDirection = Math::CalculateRayDirection(mousePos, viewMatrix, projectionMatrix, width, height);
	float rayLength = 1000.0f;
	Vector3 rayDiff = rayDirection * rayLength;

	int32_t selectedId = -1;
	float minDistance = FLT_MAX;

	for (auto [id, object] : objects_) {
		// 論理削除済みのオブジェクトは飛ばす
		if (!object->IsActive()) {
			continue; 
		}

		AABB aabb = object->GetSelectObjectAABB();
		Segment segment = Segment(rayOrigin, rayDiff);

		float distance = 0.0f;
		// レイとAABBの交差判定関数
		CollisionResult result = IsAABBSegmentCollision(aabb, segment);

		if (result.isHit) {
			// 離れている距離を取得
			Vector3 diff = rayOrigin - result.contactPosition;
			distance = diff.Length();

			// 距離が近ければidを設定
			if (distance < minDistance) {
				minDistance = distance;
				selectedId = id;
			}
		}
	}
	return selectedId;
}
#include "StaticGameObjectManager.h"
#include "StaticGameObject.h"
using namespace GameEngine;

void StaticGameObjectManager::Initialize(GameObjectManager* objectManager, ModelManager* modelManager) {
	objectManager_ = objectManager;
	modelManager_ = modelManager;

}

void StaticGameObjectManager::AddObject(std::string objecctName, std::string modelName) {

	// モデルを取得
	auto* model = modelManager_->GetNameByModel(modelName);

	objectManager_->AddObject<StaticGameObject>(objecctName, model);
}
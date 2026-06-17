#pragma once
#include "GameObjectManager.h"
#include "ModelManager.h"

namespace GameEngine {

	// オブジェクト配置するためのデータ
	struct ObjectSpawnData {
		std::string objectName_ = "None";
		std::string modelName_ = "None";
	};

	class StaticGameObjectManager {
	public:
		StaticGameObjectManager() = default;
		~StaticGameObjectManager() = default;

		void Initialize(GameObjectManager* objectManager, ModelManager* modelManager);
		
		void AddObject(std::string objecctName, std::string modelName);


	private:
		// オブジェクト管理
		GameObjectManager* objectManager_ = nullptr;
		// モデル管理
		ModelManager* modelManager_ = nullptr;
	};
}

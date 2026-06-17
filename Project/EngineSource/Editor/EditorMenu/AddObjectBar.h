#pragma once
#include "StaticGameObjectManager.h"

namespace GameEngine {

	class AddObjectBar {
	public:
		AddObjectBar(StaticGameObjectManager* staticObjectManager, RenderQueue* renderQueue);

		void Run();

	private:
		// 配置オブジェクト管理
		StaticGameObjectManager* staticObjectManager_ = nullptr;

		RenderQueue* renderQueue_ = nullptr;

		// 選択中のオブジェクト
		StaticGameObject* selectObject_ = nullptr;
	};
}

#pragma once
#include "StaticGameObjectManager.h"

namespace GameEngine {

	class DebugCamera;

	class AddObjectBar {
	public:
		AddObjectBar(StaticGameObjectManager* staticObjectManager, RenderQueue* renderQueue, DebugCamera* debugCamera);

		void Run();

		void ApplyGuizmo();

		void AddObjectFromPath(const std::string& filePath);

	private:
		// 配置オブジェクト管理
		StaticGameObjectManager* staticObjectManager_ = nullptr;
		RenderQueue* renderQueue_ = nullptr;
		DebugCamera* debugCamera_ = nullptr;

		// 選択中のオブジェクト
		StaticGameObject* selectObject_ = nullptr;
		int32_t selectedId_ = -1;
	};
}

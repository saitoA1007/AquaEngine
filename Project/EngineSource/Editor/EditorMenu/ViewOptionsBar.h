#pragma once
#include "WorldTransform.h"

namespace GameEngine {

	class Input;
	class RenderQueue;
	class Model;
	class DebugCamera;

	class ViewOptionsBar {
	public:
		ViewOptionsBar(Input* input, RenderQueue* renderQueue, Model* gridModel);

		void Run();

	private:
		// デバック画面の描画
		bool isDebugView_ = true;

		// デバック描画を表示
		bool isDebugDraw_ = true;

		RenderQueue* renderQueue_ = nullptr;

		// グリッドを描画するためのモデル
		Model* gridModel_;
		WorldTransform gridWorldTransform_;

		// デバックカメラ
		std::unique_ptr<DebugCamera> debugCamera_;
	};
}

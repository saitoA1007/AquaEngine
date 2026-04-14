#pragma once
#include "WorldTransform.h"
#include "WorldTransforms.h"

namespace GameEngine {

	// 前方宣言
	class RenderQueue;
	class Model;
	class Camera;

	class ModelComponent {
	public:
		// モデルデータ
		ModelComponent(Model* model);

		// 静的初期化
		static void SetRenderQueue(RenderQueue* queue) { renderQueue_ = queue; }

		// マテリアルデータを設定
		void SetMaterial(GpuResource* material) {
			material_ = material;
		}

	private:
		static RenderQueue* renderQueue_;
		static Camera* mainCamera_;

		// モデルデータ
		Model* model_ = nullptr;

		// モデルが持つワールド行列
		WorldTransform worldTransform_;

		// マテリアルデータ
		GpuResource* material_ = nullptr;

		uint32_t texture_ = 0;
	};
}


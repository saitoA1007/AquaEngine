#pragma once
#include "WorldTransform.h"
#include "WorldTransforms.h"

namespace GameEngine {

	// 前方宣言
	class Model;

	class ModelComponent {
	public:
		// モデルデータ
		ModelComponent(Model* model);

		// マテリアルデータを設定
		void SetMaterial(GpuResource* material) {
			material_ = material;
		}

	private:
		// モデルデータ
		Model* model_ = nullptr;

		// モデルが持つワールド行列
		WorldTransform worldTransform_;

		// マテリアルデータ
		GpuResource* material_ = nullptr;

		uint32_t texture_ = 0;
	};
}


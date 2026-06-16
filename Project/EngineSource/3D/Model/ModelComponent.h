#pragma once
#include <unordered_map>
#include "DrawRequest.h"
#include "WorldTransform.h"
#include "Material.h"
#include "RefBuffer.h"

namespace GameEngine {

	// 前方宣言
	class RenderQueue;

	/// <summary>
	/// モデルのデータから加工をおこなう
	/// </summary>
	class ModelComponent {
	public:
		// モデルデータ
		ModelComponent(Model* model);

		// 更新処理
		void Update();

		// 描画処理
		void Draw(RenderQueue* renderQueue, const Draw3dType& drawType = Draw3dType::Default,const std::string& passName = "DefaultPass");

		// レイトレによる描画
		void DrawRaytracing(RenderQueue* renderQueue);

		// 参照値を設定
		void SetRefType(uint32_t type) {
			refBuffer_.SetType(type);
		}

	public:

		// モデルが持つワールド行列
		WorldTransform worldTransform_;

		// マテリアルデータ
		Material::MaterialData* materialData_ = nullptr;

	private:
		// モデルデータ
		Model* model_ = nullptr;

		// 標準のマテリアル
		Material defaultMaterial_;

		// 参照用
		RefBuffer refBuffer_;
	};
}


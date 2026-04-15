#pragma once
#include <unordered_map>
#include "DrawRequest.h"
#include "WorldTransform.h"
#include "Material.h"

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
		void Draw(RenderQueue* renderQueue, const DrawType& drawType = DrawType::Default,const std::string& passName = "DefaultPass");

	public:

		// モデルが持つワールド行列
		WorldTransform worldTransform_;

		// 色
		Vector4 color_ = {1.0f,1.0f,1.0f,1.0f};

		// 使用する画像
		uint32_t texture_ = 0;

		// 輝度
		float shininess_ = 500.0f;

		// ライトの適応
		bool isEnableLighting_ = false;

	private:
		// モデルデータ
		Model* model_ = nullptr;

		// 標準のマテリアル
		Material defaultMaterial_;
	};
}


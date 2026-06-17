#pragma once
#include "Geometry.h"
#include "IGameObject.h"
#include "ModelComponent.h"

namespace GameEngine {

	class StaticGameObject : public IGameObject {
	public:
		StaticGameObject(std::string name, Model* model);
		~StaticGameObject() = default;

		//void Initialize() override;
		void Update() override;
		void DebugUpdate() override;
		void Draw() override;

	public:

		// ワールド行列を取得
		WorldTransform& GetWorldTransform() { return modelComponent_.worldTransform_; }

		// オブジェクト選択用のAABBの当たり判定
		AABB GetSelectObjectAABB() const { return AABB({ -1.0f,-1.0f,-1.0f }, { 1.0f,1.0f,1.0f }); }

	private:
		ModelComponent modelComponent_;

		std::string name_ = "None";

	};
}

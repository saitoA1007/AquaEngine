#pragma once
#include "IGameObject.h"
#include "ModelComponent.h"

namespace GameEngine {

	class StaticGameObject : public IGameObject {
	public:
		StaticGameObject(std::string name, Model* model);
		~StaticGameObject() = default;

		//void Initialize() override;
		void Update() override;
		void Draw() override;

	public:

		// ワールド行列を取得
		WorldTransform& GetWorldTransform() { return modelComponent_.worldTransform_; }

	private:
		ModelComponent modelComponent_;

		std::string name_ = "None";

	};
}

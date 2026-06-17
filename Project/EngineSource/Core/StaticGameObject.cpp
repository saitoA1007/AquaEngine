#include "StaticGameObject.h"

using namespace GameEngine;

StaticGameObject::StaticGameObject(std::string name, Model* model) : modelComponent_(model) {

	name_ = name;

}

void StaticGameObject::Update() {
	// 更新
	modelComponent_.Update();
}

void StaticGameObject::DebugUpdate() {
	// 更新
	modelComponent_.Update();
}

void StaticGameObject::Draw() {
	// 描画
	modelComponent_.DrawRaytracing(renderQueue_);
}
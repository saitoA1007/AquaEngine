#include "StaticGameObject.h"

using namespace GameEngine;

StaticGameObject::StaticGameObject(std::string name, std::string modelName, Model* model) : modelComponent_(model) {
	name_ = name;
	modelName_ = modelName;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>(name_);
	debugParame_->RegisterWorld("World", modelComponent_.worldTransform_);
}

void StaticGameObject::Update() {

	debugParame_->ApplyIfDirty();

	// 更新
	modelComponent_.Update();
}

void StaticGameObject::DebugUpdate() {
	// デバック状態でも更新をおこなう
	Update();
}

void StaticGameObject::Draw() {
	// 描画
	modelComponent_.DrawRaytracing(renderQueue_);
}
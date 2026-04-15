#include "ModelComponent.h"
#include "Model.h"
#include "RenderQueue.h"
using namespace GameEngine;

ModelComponent::ModelComponent(Model* model) {
	model_ = model;
	worldTransform_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} });

	// モデルからマテリアル情報を取得
	Material* material = model_->GetMaterial("default");
	auto data = material->GetMaterialData();

	color_ = data->color;
	shininess_ = data->shininess;
	isEnableLighting_ = data->enableLighting;
	texture_ = data->textureHandle;
	// マテリアルを作成
	defaultMaterial_.Initialize(color_, { 1.0f,1.0f,1.0f }, shininess_, isEnableLighting_);
}

void ModelComponent::Update() {
	
	// 行列を更新
	worldTransform_.UpdateTransformMatrix();

	// マテリアルを更新
	defaultMaterial_.SetColor(color_);
	defaultMaterial_.SetShininess(shininess_);
	defaultMaterial_.SetEnableLighting(isEnableLighting_);
	defaultMaterial_.SetTextureHandle(texture_);
}

void ModelComponent::Draw(RenderQueue* renderQueue, const DrawType& drawType, const std::string& passName) {

	switch (drawType)
	{
	case GameEngine::DrawType::Default:
		renderQueue->SubmitModel(model_, worldTransform_,color_.w, defaultMaterial_.GetConstantBuffer(), passName);
		break;

	case GameEngine::DrawType::DefaultAdd:
		renderQueue->SubmitModel(model_, worldTransform_, color_.w, defaultMaterial_.GetConstantBuffer(), passName);
		break;

	case GameEngine::DrawType::Animation:
		renderQueue->SubmitAnimation(model_, worldTransform_, color_.w, defaultMaterial_.GetConstantBuffer(), passName);
		break;

	case GameEngine::DrawType::ShadowMap:
		renderQueue->SubmitShadowMap(model_, worldTransform_);
		break;

	case GameEngine::DrawType::Instancing:
	case GameEngine::DrawType::InstancingAdd:
	case GameEngine::DrawType::Grid:
	case GameEngine::DrawType::Skybox:
	default:
		break;
	}
}
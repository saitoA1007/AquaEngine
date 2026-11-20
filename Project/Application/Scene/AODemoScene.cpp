#include "pch.h"
#include "AODemoScene.h"
#include <cmath>
#include "ImguiManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

AODemoScene::AODemoScene() {

	// メインカメラ。部屋の角を斜め上から見下ろす
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.45f,-0.15f,0.0f},{0.0f,4.0f,-3.5f} }, 1280, 720);
	mainCamera_->Update();

	// 背景。明るい空のほうが環境光とAOの差が分かりやすい
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// モデル。映り込みでAOが見えにくくならないよう、粗い非金属にする
	cubeModel_ = modelManager_->GetNameByModel("cube.obj");
	cubeModel_->SetDefaultIsEnableLight(true);
	cubeModel_->SetDefaultColor({ 0.85f,0.85f,0.85f,1.0f });
	cubeModel_->SetDefaultMetallic(0.0f);
	cubeModel_->SetRoughness(0.9f);

	sphereModel_ = modelManager_->GetNameByModel("sphereShade.obj");
	sphereModel_->SetDefaultIsEnableLight(true);
	sphereModel_->SetDefaultColor({ 0.9f,0.6f,0.4f,1.0f });
	sphereModel_->SetDefaultMetallic(0.0f);
	sphereModel_->SetRoughness(0.7f);

	// 床
	AddBox({ 10.0f,0.25f,10.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,-0.25f,0.0f });
	// 奥の壁(手前の面がz=6)
	AddBox({ 10.0f,4.0f,0.25f }, { 0.0f,0.0f,0.0f }, { 0.0f,4.0f,6.25f });
	// 左の壁(右の面がx=-6)
	AddBox({ 0.25f,4.0f,10.0f }, { 0.0f,0.0f,0.0f }, { -6.25f,4.0f,0.0f });

	// 奥の角に積んだ箱
	AddBox({ 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { -4.8f,1.0f,4.8f });
	AddBox({ 0.7f,0.7f,0.7f }, { 0.0f,0.5f,0.0f }, { -4.6f,2.7f,4.6f });
	// 壁際の低い箱
	AddBox({ 1.2f,0.6f,0.8f }, { 0.0f,0.0f,0.0f }, { -1.8f,0.6f,5.2f });
	// 柱
	AddBox({ 0.5f,3.0f,0.5f }, { 0.0f,0.0f,0.0f }, { 3.5f,3.0f,3.5f });

	// 床に置いた球と、それに寄り添う小さな球
	AddSphere(1.0f, { 0.0f,1.0f,0.0f });
	AddSphere(0.6f, { 1.55f,0.6f,0.3f });
	// 左の壁に接する球
	AddSphere(0.8f, { -5.2f,0.8f,0.5f });

	// 壁へ近づいたり離れたりする球
	movingSphere_.Initialize({ {0.7f,0.7f,0.7f},{0.0f,0.0f,0.0f},{1.5f,0.7f,4.0f} });
}

void AODemoScene::AddBox(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	auto world = std::make_unique<WorldTransform>();
	world->Initialize({ scale, rotate, translate });
	boxes_.push_back(std::move(world));
}

void AODemoScene::AddSphere(float radius, const Vector3& translate) {
	auto world = std::make_unique<WorldTransform>();
	world->Initialize({ {radius,radius,radius},{0.0f,0.0f,0.0f},translate });
	spheres_.push_back(std::move(world));
}

void AODemoScene::Initialize() {

}

void AODemoScene::Update() {

	// カメラの更新処理
	mainCamera_->Update();

	DebugUpdate();
}

void AODemoScene::DebugUpdate() {
	// 球を奥の壁へ近づけたり離したりする
	if (isMoving_) {
		moveTimer_ += FpsCounter::GetDeltaTime();
	}
	float wave = 0.5f + 0.5f * std::sin(moveTimer_ * 1.2f);
	movingSphere_.transform_.translate.z = 3.0f + 2.3f * wave;
	movingSphere_.UpdateTransformMatrix();

	// 平行光源
	auto* light = renderQueue_->GetLightManager();
	Vector3 dir = lightDir_;
	dir.Normalize();
	light->SetDirectionalDirction(dir);
	light->SetDirectionalIntensity(lightIntensity_);
	light->SetDirectionalColor(lightColor_);

#ifdef USE_IMGUI
	ImGui::Begin("AODemo");
	// ImGuiのフォントに日本語が無いため英語で表示する
	ImGui::TextWrapped("Compare with PostEffect window > Scene node > RTAO (Enabled / Debug view)");
	ImGui::Checkbox("moveSphere", &isMoving_);
	ImGui::DragFloat3("lightDir", &lightDir_.x, 0.05f);
	ImGui::DragFloat("lightIntensity", &lightIntensity_, 0.05f, 0.0f, 5.0f);
	ImGui::ColorEdit4("lightColor", &lightColor_.x);
	ImGui::End();
#endif
}

void AODemoScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 全てレイトレーシングで描画する
	for (auto& box : boxes_) {
		renderQueue_->SubmitRaytracingModel(cubeModel_, *box);
	}
	for (auto& sphere : spheres_) {
		renderQueue_->SubmitRaytracingModel(sphereModel_, *sphere);
	}
	renderQueue_->SubmitRaytracingModel(sphereModel_, movingSphere_);
}

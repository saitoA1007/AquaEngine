#include "pch.h"
#include "DemoScene.h"
#include "ImguiManager.h"
#include "PostProcess/PostEffectData.h"
#include "FPSCounter.h"
#include "ParticleBehavior.h"
#include "Application/Demo/IceDemo.h"
#include "Application/Demo/FructureDemo.h"
#include "Application/Demo/Test.h"
#include "Application/Demo/TestFructure.h"
using namespace GameEngine;

DemoScene::~DemoScene() {}

DemoScene::DemoScene() {

	// 操作方法を登録
	inputCommand_->RegisterCommand("MoveLeft", { KeyBinding{ DIK_A },PadStickBinding{ PadSide::kLeft, {-1.0f,0.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_LEFT } });
	inputCommand_->RegisterCommand("MoveRight", { KeyBinding{ DIK_D },PadStickBinding{ PadSide::kLeft, {1.0f,0.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_RIGHT } });
	inputCommand_->RegisterCommand("MoveForward", { KeyBinding{ DIK_W },PadStickBinding{ PadSide::kLeft, {0.0f,1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("MoveBack", { KeyBinding{ DIK_S },PadStickBinding{ PadSide::kLeft, {0.0f,-1.0f} }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_DOWN } });
	inputCommand_->RegisterCommand("Reassemble", { KeyBinding{ DIK_R, PressType::kTrigger } });

	// メインカメラの初期化
	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,1.0f,-15.0f} }, 1280, 720);
	mainCamera_->Update();

	debugParame_.Register("EaseCurve", easeCurve_);
	debugParame_.Register("MaxTime", maxTime_,1);
	debugParame_.Apply();

	// 背景画像を設定
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// 破片のテスト
	auto* testModel_ = modelManager_->GetNameByModel("test.gltf");
	// 破片のデモ
	gameObjectManager_->AddObject<FructureDemo>("FructureDemo", inputCommand_, testModel_);
}

void DemoScene::Initialize() {

}

void DemoScene::Update() {
	debugParame_.ApplyIfDirty();

	timer_ += FpsCounter::GetDeltaTime() / maxTime_;

	//text_->color_.w = Lerp(0.0f, 1.0f, timer_, easeCurve_);

	if (timer_ >= 1.0f) {
		timer_ = 0.0f;
	}

	if (inputCommand_->IsCommandActive("Decision")) {
		//isFinished_ = true;
		//auto* colorGrading = postEffectManager_->GetPostEffect<ColorGrading>("ColorGradingPass");
		//colorGrading->SetEnableGrayscale(false);
	}

	// カメラの更新処理
	mainCamera_->Update();

	DebugUpdate();
}

void DemoScene::DebugUpdate() {
#ifdef USE_IMGUI
	auto* light = renderQueue_->GetLightManager();

	ImGui::Begin("test");
	ImGui::DragFloat3("lightDir", &dir_.x, 0.1f);
	ImGui::DragFloat("lightIntensity", &intensity_, 0.1f);
	ImGui::ColorEdit4("lightColor", &lightColor_.x);

	dir_.Normalize();

	light->SetDirectionalDirction(dir_);
	light->SetDirectionalIntensity(intensity_);
	light->SetDirectionalColor(lightColor_);
	ImGui::End();
#endif
}

void DemoScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 破片を1つに集約していない
	//renderQueue_->SubmitModel(noFractureModel_, noFractureWorld_);

	// アニメーションモデル
	//renderQueue_->SubmitRaytracingModel(model_, world_);
}

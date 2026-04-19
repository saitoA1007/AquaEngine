#include"SceneLightingController.h"
#include"GameParamEditor.h"
using namespace GameEngine;

void SceneLightingController::Initialize() {

	// 平行光源ライト
	directionalData_.active = true;
	directionalData_.color = { 1.0f,1.0f,1.0f,1.0f };
	directionalData_.direction = { 0.0,-1.0f,0.0f };
	directionalData_.intensity = 1.0f;

	// ライトの設定
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(true, false, false);
	lightManager_->SetDirectionalData(directionalData_);

#ifdef USE_IMGUI
	// 値を登録する
	RegisterDebugParam();
#else
	// 値を適応させる
	ApplyDebugParam();
#endif
}

void SceneLightingController::Update() {
#ifdef USE_IMGUI
	// 値を適応
	ApplyDebugParam();
#endif

	lightManager_->Update();
}

void SceneLightingController::RegisterDebugParam() {
	// 登録
	//GameParamEditor::GetInstance()->AddItem("GameSceneLight", "Direction", directionalData_.direction);
	//GameParamEditor::GetInstance()->AddItem("GameSceneLight", "Intensity", directionalData_.intensity);
	//GameParamEditor::GetInstance()->AddItem("GameSceneLight", "Color", directionalData_.color);
}

void SceneLightingController::ApplyDebugParam(){
	// 適応
	//Vector3 tmpDirection = GameParamEditor::GetInstance()->GetValue<Vector3>("GameSceneLight", "Direction");
	//directionalData_.direction = Normalize(tmpDirection);
	//directionalData_.intensity = GameParamEditor::GetInstance()->GetValue<float>("GameSceneLight", "Intensity");
	//directionalData_.color = GameParamEditor::GetInstance()->GetValue<Vector4>("GameSceneLight", "Color");
	//
	//// ライトマネージャーに適応
	//lightManager_->SetDirectionalData(directionalData_);
}
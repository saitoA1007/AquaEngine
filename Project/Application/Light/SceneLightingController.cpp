#include"SceneLightingController.h"
#include"GameParamEditor.h"
using namespace GameEngine;

void SceneLightingController::Initialize(ID3D12Device* device) {

	// 平行光源ライト
	directionalData_.active = true;
	directionalData_.color = { 1.0f,1.0f,1.0f,1.0f };
	directionalData_.direction = { 0.0,-1.0f,0.0f };
	directionalData_.intensity = 1.0f;

	// ライトの設定
	lightManager_ = std::make_unique<LightManager>();
	lightManager_->Initialize(device, true, false, false);
	lightManager_->SetDirectionalData(directionalData_);

#ifdef _DEBUG
	// 値を登録する
	RegisterBebugParam();
#else
	// 値を適応させる
	ApplyDebugParam();
#endif
}

void SceneLightingController::Update() {
#ifdef _DEBUG
	// 値を適応
	ApplyDebugParam();
#endif

	lightManager_->Update();
}

void SceneLightingController::RegisterBebugParam() {
	// 登録
	GameParamEditor::GetInstance()->AddItem("GameSceneLight", "Direction", directionalData_.direction);
	GameParamEditor::GetInstance()->AddItem("GameSceneLight", "Intensity", directionalData_.intensity);
	GameParamEditor::GetInstance()->AddItem("GameSceneLight", "Color", directionalData_.color);
}

void SceneLightingController::ApplyDebugParam(){
	// 適応
	Vector3 tmpDireciton = GameParamEditor::GetInstance()->GetValue<Vector3>("GameSceneLight", "Direction");
	directionalData_.direction = Normalize(tmpDireciton);
	directionalData_.intensity = GameParamEditor::GetInstance()->GetValue<float>("GameSceneLight", "Intensity");
	directionalData_.color = GameParamEditor::GetInstance()->GetValue<Vector4>("GameSceneLight", "Color");

	// ライトマネージャーに適応
	lightManager_->SetDirectionalData(directionalData_);
}
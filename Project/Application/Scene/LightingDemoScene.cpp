#include "pch.h"
#include "LightingDemoScene.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include "ImguiManager.h"
#include "FPSCounter.h"
using namespace GameEngine;

namespace {
	// 前後の空白を取り除く
	std::string Trim(const std::string& str) {
		const size_t begin = str.find_first_not_of(" \t\r\n");
		if (begin == std::string::npos) { return ""; }
		const size_t end = str.find_last_not_of(" \t\r\n");
		return str.substr(begin, end - begin + 1);
	}
}

LightingDemoScene::LightingDemoScene() {

	// 操作方法を登録
	inputCommand_->RegisterCommand("LightingDemoPrev", { KeyBinding{ DIK_A, PressType::kTrigger }, KeyBinding{ DIK_LEFT, PressType::kTrigger }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_LEFT, PressType::kTrigger } });
	inputCommand_->RegisterCommand("LightingDemoNext", { KeyBinding{ DIK_D, PressType::kTrigger }, KeyBinding{ DIK_RIGHT, PressType::kTrigger }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_RIGHT, PressType::kTrigger } });
	inputCommand_->RegisterCommand("LightingDemoZoomIn", { KeyBinding{ DIK_W }, KeyBinding{ DIK_UP }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_UP } });
	inputCommand_->RegisterCommand("LightingDemoZoomOut", { KeyBinding{ DIK_S }, KeyBinding{ DIK_DOWN }, PadButtonBinding{ XINPUT_GAMEPAD_DPAD_DOWN } });

	mainCamera_ = std::make_unique<Camera>();
	mainCamera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,std::numbers::pi_v<float>,0.0f},{0.0f,1.0f,cameraDistance_} }, 1280, 720);
	mainCamera_->Update();

	// 背景
	uint32_t skyboxGH = textureManager_->GetHandleByName("grasslands_sunset_1k.dds");
	renderQueue_->SetSkyboxTexture(skyboxGH);

	// 部屋に置く球
	sphereModel_ = modelManager_->GetNameByModel("sphereShade.obj");

	// よく使うマテリアル
	const MaterialParam mirror = { {0.95f,0.95f,0.95f,1.0f}, 1.0f, 0.02f, 1.0f };
	const MaterialParam glass = { {0.95f,0.97f,1.0f,0.1f}, 0.0f, 0.02f, 1.5f };

	// 基準となる拡散反射のみの部屋
	{
		Room& room = AddRoom("CornellBox-Original.obj", "Original",
			"Diffuse only. Reference for the other rooms.");
		(void)room;
	}

	// 背の高い箱を鏡にした部屋
	{
		Room& room = AddRoom("CornellBox-Mirror.obj", "Mirror",
			"Tall box is a perfect mirror (metallic 1 / roughness 0.02).");
		SetRoomMaterial(room, "tallBox", mirror);
	}

	// 光沢のある球と箱
	{
		Room& room = AddRoom("CornellBox-Glossy.obj", "Glossy",
			"Glossy sphere and box. Blurred reflections from roughness.");
		SetRoomMaterial(room, "sphere", { {0.486f,0.631f,0.663f,1.0f}, 0.5f, 0.1f, 1.0f });
		SetRoomMaterial(room, "shortBox", { {0.525f,0.51f,0.48f,1.0f}, 0.4f, 0.25f, 1.0f });
	}

	// 磨かれた床
	{
		Room& room = AddRoom("CornellBox-Glossy-Floor.obj", "Glossy Floor",
			"Polished floor reflecting the walls, box and sphere.");
		SetRoomMaterial(room, "floor", { {0.725f,0.71f,0.68f,1.0f}, 0.5f, 0.05f, 1.0f });
		SetRoomMaterial(room, "sphere", { {0.486f,0.631f,0.663f,1.0f}, 0.3f, 0.2f, 1.0f });
	}

	// 鏡の球とガラスの球
	{
		Room& room = AddRoom("CornellBox-Sphere.obj", "Mirror & Glass Sphere",
			"Left: mirror sphere (reflection). Right: glass sphere, IOR 1.5 (refraction).");
		SetRoomMaterial(room, "leftSphere", mirror);
		SetRoomMaterial(room, "rightSphere", glass);
	}

	// 水面
	{
		Room& room = AddRoom("CornellBox-Water.obj", "Water",
			"Water surface, IOR 1.33. Spheres seen through the water are distorted.");
		SetRoomMaterial(room, "leftSphere", mirror);
		SetRoomMaterial(room, "rightSphere", glass);
		SetRoomMaterial(room, "water", { {0.6f,0.85f,1.0f,0.25f}, 0.0f, 0.05f, 1.33f });
	}

	// 粗さの比較。左から右へ粗くなる
	{
		Room& room = AddRoom("CornellBox-Empty-RG.obj", "Roughness",
			"Roughness 0.02 -> 0.8 (left to right). Back: gold metal, front: plastic.");
		const float roughness[] = { 0.02f, 0.2f, 0.45f, 0.8f };
		for (int i = 0; i < 4; ++i) {
			// 画面の左が+x側
			const float x = 0.6f - 0.4f * static_cast<float>(i);
			AddSphere(room, 0.17f, { x,0.17f,-0.35f }, { {1.0f,0.78f,0.34f,1.0f}, 1.0f, roughness[i], 1.0f });
			AddSphere(room, 0.17f, { x,0.17f,0.35f }, { {0.2f,0.35f,0.8f,1.0f}, 0.0f, roughness[i], 1.0f });
		}
	}

	// 屈折率の比較。左から右へ屈折率が大きくなる
	{
		Room& room = AddRoom("CornellBox-Empty-CO.obj", "Index of Refraction",
			"Glass spheres, IOR 1.0 / 1.33 (water) / 1.5 (glass) / 2.4 (diamond).");
		const float ior[] = { 1.0f, 1.33f, 1.5f, 2.4f };
		for (int i = 0; i < 4; ++i) {
			const float x = 0.66f - 0.44f * static_cast<float>(i);
			MaterialParam param = glass;
			param.ior = ior[i];
			AddSphere(room, 0.2f, { x,0.8f,0.0f }, param);
		}
	}

	// 合わせ鏡。反射の再帰を確認する
	{
		Room& room = AddRoom("CornellBox-Empty-White.obj", "Facing Mirrors",
			"Left and right walls are mirrors. Shows recursive reflections.");
		const MaterialParam wallMirror = { {0.9f,0.9f,0.9f,1.0f}, 1.0f, 0.0f, 1.0f };
		SetRoomMaterial(room, "leftWall", wallMirror);
		SetRoomMaterial(room, "rightWall", wallMirror);
		AddSphere(room, 0.3f, { 0.3f,0.3f,-0.3f }, { {0.8f,0.15f,0.1f,1.0f}, 0.0f, 0.4f, 1.0f });
		AddSphere(room, 0.25f, { -0.35f,0.25f,0.3f }, glass);
	}

	cameraX_ = GetRoomX(currentRoom_);
}

LightingDemoScene::Room& LightingDemoScene::AddRoom(const std::string& modelName, const std::string& name, const std::string& description) {
	auto room = std::make_unique<Room>();
	room->name = name;
	room->description = description;
	room->model = modelManager_->GetNameByModel(modelName);
	room->world.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{GetRoomX(rooms_.size()),0.0f,0.0f} });

	// 映り込みが分かりやすいよう、まず全体を粗い非金属にする
	for (auto& [materialName, material] : room->model->GetMaterials()) {
		material->SetMetallic(0.0f);
		material->SetRoughness(0.9f);
		material->SetIOR(1.0f);
		material->SetEnableLighting(true);
	}

	// 天井の照明は発光体として、ライティングせずに明るい色で表示する
	for (auto& [materialName, material] : room->model->GetMaterials()) {
		if (Trim(materialName) == "light") {
			material->SetEnableLighting(false);
			material->SetColor({ 1.0f,0.92f,0.8f,1.0f });
		}
	}

	rooms_.push_back(std::move(room));
	return *rooms_.back();
}

void LightingDemoScene::SetRoomMaterial(Room& room, const std::string& materialName, const MaterialParam& param) {
	for (auto& [name, material] : room.model->GetMaterials()) {
		if (Trim(name) != materialName) { continue; }
		material->SetColor(param.color);
		material->SetMetallic(param.metallic);
		material->SetRoughness(param.roughness);
		material->SetIOR(param.ior);
	}
}

void LightingDemoScene::AddSphere(Room& room, float radius, const Vector3& localPos, const MaterialParam& param) {
	auto sphere = std::make_unique<ModelComponent>(sphereModel_);

	const Vector3& roomPos = room.world.transform_.translate;
	sphere->worldTransform_.Initialize({ {radius,radius,radius},{0.0f,0.0f,0.0f},
		{roomPos.x + localPos.x, roomPos.y + localPos.y, roomPos.z + localPos.z} });

	// 個体ごとのマテリアルを設定
	auto* data = sphere->materialData_;
	data->color = param.color;
	data->metallic = param.metallic;
	data->roughness = param.roughness;
	data->ior = param.ior;
	data->enableLighting = true;

	// 半透明であれば屈折する描画にする
	if (param.color.w < 1.0f) {
		sphere->SetRefType(static_cast<uint32_t>(BufferType::kDefalutMaterialTranslucent));
	}

	sphere->Update();
	room.objects.push_back(std::move(sphere));
}

float LightingDemoScene::GetRoomX(size_t index) const {
	return -kRoomSpacing_ * static_cast<float>(index);
}

void LightingDemoScene::Initialize() {

}

void LightingDemoScene::Update() {
	const float deltaTime = FpsCounter::GetDeltaTime();
	const size_t roomCount = rooms_.size();

	// A,Dで隣の部屋へ移動する。端まで行ったら反対側へ戻る
	if (inputCommand_->IsCommandActive("LightingDemoPrev")) {
		currentRoom_ = (currentRoom_ + roomCount - 1) % roomCount;
	}
	if (inputCommand_->IsCommandActive("LightingDemoNext")) {
		currentRoom_ = (currentRoom_ + 1) % roomCount;
	}

	// W,Sで拡大、縮小
	const float kZoomSpeed = 1.2f;
	if (inputCommand_->IsCommandActive("LightingDemoZoomIn")) {
		targetCameraDistance_ *= std::exp(-kZoomSpeed * deltaTime);
	}
	if (inputCommand_->IsCommandActive("LightingDemoZoomOut")) {
		targetCameraDistance_ *= std::exp(kZoomSpeed * deltaTime);
	}
	targetCameraDistance_ = std::clamp(targetCameraDistance_, kMinCameraDistance_, kMaxCameraDistance_);

	// 目標へ滑らかに近づける
	const float follow = 1.0f - std::exp(-8.0f * deltaTime);
	cameraX_ += (GetRoomX(currentRoom_) - cameraX_) * follow;
	cameraDistance_ += (targetCameraDistance_ - cameraDistance_) * follow;

	mainCamera_->transform_.translate = { cameraX_, 1.0f, cameraDistance_ };
	mainCamera_->Update();

	// 平行光源
	auto* light = renderQueue_->GetLightManager();
	Vector3 dir = lightDir_;
	dir.Normalize();
	light->SetDirectionalDirction(dir);
	light->SetDirectionalIntensity(lightIntensity_);
	light->SetDirectionalColor(lightColor_);

	DebugUpdate();
}

void LightingDemoScene::DebugUpdate() {
#ifdef USE_IMGUI
	const Room& room = *rooms_[currentRoom_];

	ImGui::Begin("LightingDemo");
	// ImGuiのフォントに日本語が無いため英語で表示する
	ImGui::Text("Room %d / %d : %s", static_cast<int>(currentRoom_) + 1, static_cast<int>(rooms_.size()), room.name.c_str());
	ImGui::TextWrapped("%s", room.description.c_str());
	ImGui::Separator();
	ImGui::TextWrapped("A / D : previous / next room   W / S : zoom in / out");
	ImGui::Separator();
	ImGui::DragFloat3("lightDir", &lightDir_.x, 0.05f);
	ImGui::DragFloat("lightIntensity", &lightIntensity_, 0.05f, 0.0f, 5.0f);
	ImGui::ColorEdit4("lightColor", &lightColor_.x);
	ImGui::End();
#endif
}

void LightingDemoScene::Draw() {

	// 描画に使用するカメラを設定
	renderQueue_->SetCamera(mainCamera_.get());

	// 部屋同士の映り込みも見えるよう、全ての部屋をレイトレーシングで描画する
	for (auto& room : rooms_) {
		renderQueue_->SubmitRaytracingModel(room->model, room->world);
		for (auto& object : room->objects) {
			object->DrawRaytracing(renderQueue_);
		}
	}
}

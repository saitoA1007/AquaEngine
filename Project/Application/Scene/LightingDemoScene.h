#pragma once
#include <memory>
#include <string>
#include <vector>
#include "IScene.h"

// エンジン機能をインクルード
#include "Camera.h"
#include "Model.h"
#include "ModelComponent.h"
#include "WorldTransform.h"
#include "IceMaterial.h"

#include "Application/Scene/Transition/Fade.h"

/// <summary>
/// イティングを確認するためのデモシーン。
/// </summary>
class LightingDemoScene : public GameEngine::IScene {
public:
	LightingDemoScene();
	~LightingDemoScene() = default;

	void Initialize() override;

	void Update() override;

	void DebugUpdate() override;

	void Draw() override;

	std::string NextSceneName() override { return "Game"; }

	std::unique_ptr<ITransitionEffect> GetTransitionEffect() override { return std::make_unique<Fade>(); }

private:

	// 1つの部屋
	struct Room {
		std::string name;        // 部屋の名前
		std::string description; // 確認できる内容
		GameEngine::Model* model = nullptr; // コーネルボックスのモデル
		GameEngine::WorldTransform world;   // 部屋の位置
		std::vector<std::unique_ptr<GameEngine::ModelComponent>> objects; // 部屋に追加で置く球
		std::vector<std::unique_ptr<GameEngine::IceMaterial>> iceMaterials; // 氷のオブジェクトが参照するマテリアル
	};

	// マテリアルの設定値
	struct MaterialParam {
		Vector4 color = { 1.0f,1.0f,1.0f,1.0f };
		float metallic = 0.0f;
		float roughness = 0.9f;
		float ior = 1.0f;
	};

private:

	// 部屋を追加する。i番目の部屋は画面右方向へ並ぶ
	Room& AddRoom(const std::string& modelName, const std::string& name, const std::string& description);

	// 部屋のモデルが持つマテリアルを設定する。見つからなければ何もしない
	void SetRoomMaterial(Room& room, const std::string& materialName, const MaterialParam& param);

	// 部屋に球を置く。位置は部屋のローカル座標
	void AddSphere(Room& room, float radius, const Vector3& localPos, const MaterialParam& param);

	// 部屋に透明な氷を置く。位置は部屋のローカル座標
	void AddIce(Room& room, GameEngine::Model* model, const Vector3& scale, const Vector3& rotate, const Vector3& localPos, const Vector4& color);

	// 部屋の中心のx座標
	float GetRoomX(size_t index) const;

private:

	// メインカメラ
	std::unique_ptr<GameEngine::Camera> mainCamera_;

	// 部屋に置く球のモデル
	GameEngine::Model* sphereModel_ = nullptr;
	// 氷のブロックに使う立方体のモデル
	GameEngine::Model* cubeModel_ = nullptr;

	// 並べた部屋
	std::vector<std::unique_ptr<Room>> rooms_;
	// 現在見ている部屋
	size_t currentRoom_ = 0;

	// 部屋同士の間隔
	const float kRoomSpacing_ = 3.0f;

	// カメラの位置
	float cameraX_ = 0.0f;
	// 部屋の中心からカメラまでの距離。W,Sで変える
	float cameraDistance_ = 5.0f;
	float targetCameraDistance_ = 5.0f;
	const float kMinCameraDistance_ = 1.5f;
	const float kMaxCameraDistance_ = 14.0f;

	// 平行光源。コーネルボックスは手前が開いているので、カメラ側から奥へ照らす
	Vector3 lightDir_ = { -0.25f, -0.45f, -1.0f };
	float lightIntensity_ = 1.2f;
	Vector4 lightColor_ = { 1.0f, 0.96f, 0.9f, 1.0f };
};

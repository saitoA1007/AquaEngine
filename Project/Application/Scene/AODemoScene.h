#pragma once
#include "IScene.h"

// エンジン機能をインクルード
#include "Camera.h"
#include "Model.h"
#include "WorldTransform.h"

#include "Application/Scene/Transition/Fade.h"

/// <summary>
/// RTAO(レイトレースによるアンビエントオクルージョン)を確認するためのデモシーン。
/// 床・壁の角、積んだ箱、床に置いた球など、遮蔽が起きやすい配置にしている。
/// PostEffectウィンドウのSceneノードで、RTAOの Enabled / Debug view を切り替えると違いが分かる
/// </summary>
class AODemoScene : public GameEngine::IScene {
public:
	AODemoScene();
	~AODemoScene() = default;

	void Initialize() override;

	void Update() override;

	void DebugUpdate() override;

	void Draw() override;

	std::string NextSceneName() override { return "Game"; }

	std::unique_ptr<ITransitionEffect> GetTransitionEffect() override { return std::make_unique<Fade>(); }

private:

	// 箱(立方体)を配置する
	void AddBox(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

	// 球を配置する
	void AddSphere(float radius, const Vector3& translate);

private:

	// メインカメラ
	std::unique_ptr<GameEngine::Camera> mainCamera_;

	// 使用するモデル
	GameEngine::Model* cubeModel_ = nullptr;
	GameEngine::Model* sphereModel_ = nullptr;

	// 配置したオブジェクト
	std::vector<std::unique_ptr<GameEngine::WorldTransform>> boxes_;
	std::vector<std::unique_ptr<GameEngine::WorldTransform>> spheres_;

	// 壁へ近づいたり離れたりする球(AOの追従と時間方向の蓄積を確認する)
	GameEngine::WorldTransform movingSphere_;
	bool isMoving_ = true;
	float moveTimer_ = 0.0f;

	// 平行光源。奥の壁の向こうから照らし、部屋の角を影に入れてAOが見えやすいようにする
	Vector3 lightDir_ = { 0.35f, -0.7f, -0.6f };
	float lightIntensity_ = 0.3f;
	Vector4 lightColor_ = { 1.0f, 0.96f, 0.9f, 1.0f };
};

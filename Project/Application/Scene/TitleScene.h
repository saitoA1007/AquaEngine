#pragma once
#include"IScene.h"

// エンジン機能をインクルード
#include "Camera.h"
#include "Model.h"
#include "WorldTransform.h"
#include "Animator.h"
#include "ParticleSystem/ParticleBehavior.h"

#include "Application/Scene/Transition/Fade.h"

class TitleScene : public GameEngine::IScene {
public:
	TitleScene();
	~TitleScene();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="input"></param>
	void Initialize() override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// デバック時、処理して良いものを更新する
	/// </summary>
	void DebugUpdate() override;

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw() override;

	/// <summary>
	/// 終了したことを伝える
	/// </summary>
	/// <returns></returns>
	bool IsFinished() override { return isFinished_; };

	/// <summary>
	/// 次のシーン遷移する場面の名前を取得
	/// </summary>
	/// <returns></returns>
	std::string NextSceneName() override { return "Game"; }

	/// <summary>
	/// 遷移する演出
	/// </summary>
	/// <returns></returns>
	std::unique_ptr<ITransitionEffect> GetTransitionEffect() override { return std::make_unique<Fade>(); }

private: // シーン機能

	// 終了フラグ
	bool isFinished_ = false;

	// メインカメラ
	std::unique_ptr<GameEngine::Camera> mainCamera_;

	// プリミティブのエフェクト
	GameEngine::ParticleBehavior primitiveEffect_;
	GameEngine::Model* effectModel_ = nullptr;

	GameEngine::Model* model_;
	GameEngine::WorldTransform world_;
	// アニメーションデータ
	std::map<std::string, AnimationData> cubeAnimationData_;
	// アニメーションを再生するクラス
	std::unique_ptr<GameEngine::Animator> cubeAnimator_;

	GameEngine::Model* model1_;
	GameEngine::WorldTransform world1_;

	GameEngine::Model* model2_;
	GameEngine::WorldTransform world2_;
	GameEngine::WorldTransform world3_;
	
	Vector4 color_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector4 color1_ = { 1.0f,1.0f,1.0f,1.0f };
	float metalic_ = 0.01f;
	float metalic1_ = 0.01f;
	float metalic2_ = 0.01f;
	float shininess_ = 1.0f;
	float shininess1_ = 1.0f;
	float shininess2_ = 1.0f;
	float ior_ = 1.0f;
	float ior1_ = 1.0f;

	float intensity_ = 1.0f;
	Vector3 dir_ = { 0.0f,-1.0f,0.0f };
};
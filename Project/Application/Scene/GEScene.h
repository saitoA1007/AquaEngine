#pragma once
#include"BaseScene.h"

// エンジン機能をインクルード
#include"Camera.h"
#include"DebugCamera.h"
#include"Model.h"
#include"WorldTransform.h"
#include"Sprite.h"
#include"DebugRenderer.h"

#include"Application/Scene/Transition/Fade.h"

#include"Application/Player/Player.h"
#include"Application/Camera/CameraController.h"

#include"ParticleSystem/ParticleBehavior.h"

class GEScene : public BaseScene {
public:

	~GEScene();

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="input"></param>
	void Initialize(SceneContext* context) override;

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update() override;

	/// <summary>
	/// デバック時、処理して良いものを更新する
	/// </summary>
	void DebugUpdate() override{}

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw(const bool& isDebugView) override;

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

private: // エンジンの低レイヤー機能を取得

	// デバック描画するリスト
	std::unique_ptr<GameEngine::DebugRenderer> debugRenderer_;

private: // シーン機能

	// 終了フラグ
	bool isFinished_ = false;

	// メインカメラ
	std::unique_ptr<GameEngine::Camera> mainCamera_;

	// 自キャラのモデル
	GameEngine::Model* playerModel_;
	// プレイヤー
	std::unique_ptr<Player> player_;

	// カメラコントローラークラス
	std::unique_ptr<CameraController> cameraController_;

	// uvChecker
	std::unique_ptr<GameEngine::Sprite> sprite_;
	uint32_t uvCheckerGH_ = 0;

	// パーティクル
	std::unique_ptr<GameEngine::ParticleBehavior> testParticle_;

	// 平面モデル
	GameEngine::Model* planeModel_;

private:

	/// <summary>
	/// 入力のコマンドを設定する
	/// </summary>
	void InputRegisterCommand();
};
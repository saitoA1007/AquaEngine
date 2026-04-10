#pragma once
#include"BaseScene.h"

// エンジン機能をインクルード
#include"Camera.h"
#include"DebugCamera.h"
#include"Model.h"
#include"WorldTransform.h"

#include"Application/Scene/Transition/Fade.h"

class TitleScene : public BaseScene {
public:

	~TitleScene();

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

private: // シーン機能

	// 終了フラグ
	bool isFinished_ = false;

	// メインカメラ
	std::unique_ptr<GameEngine::Camera> mainCamera_;

	// グリッドを描画するためのモデル
	GameEngine::Model* gridModel_;
	GameEngine::WorldTransform gridWorldTransform_;
};
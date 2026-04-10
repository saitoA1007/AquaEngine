#pragma once
#include <string>
#include"SceneContext.h"
#include"SceneRegistry.h"
#include"SceneTransition.h"
#include"BaseScene.h"
#include"Camera.h"

/// <summary>
/// シーンの管理
/// </summary>
class SceneManager {
public:
	SceneManager() = default;
	~SceneManager();

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <param name="context"></param>
	void Initialize(SceneContext* context, SceneRegistry* sceneRegistry);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// デバック中でも動かせるようにするシーンの更新処理
	/// </summary>
	void DebugSceneUpdate();

	/// <summary>
	/// デバック時の更新処理
	/// </summary>
	void DebugUpdate();

	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();

	/// <summary>
	/// シーンの切り替え処理
	/// </summary>
	/// <param name="sceneName">切り替え先のシーン名</param>
	void ChangeScene(const std::string& sceneName);

	/// <summary>
	/// 現在のシーンをリセットする
	/// </summary>
	void ResetCurrentScene();

	/// <summary>
	/// 現在のシーン名を取得する
	/// </summary>
	/// <returns></returns>
	const std::string& GetCurrentSceneName() const { return currentSceneName_; }

private: // エンジン機能

	// エンジン機能群
	SceneContext* context_ = nullptr;

	// シーン機能
	SceneRegistry* sceneRegistry_ = nullptr;

private: // シーン機能

	// 現在のシーン
	std::unique_ptr<BaseScene> currentScene_;

	// シーンの切り替え処理をしているか判断する
	bool isChangeScene_ = false;

	// 現在のシーン名
	std::string currentSceneName_;

	// デバックカメラ
	std::unique_ptr<GameEngine::DebugCamera> debugCamera_;

	// グリッドを描画するためのモデル
	GameEngine::Model* gridModel_;
	GameEngine::WorldTransform gridWorldTransform_;

	// デバックカメラとの切り替えを管理
	bool isDebugView_ = true;

	// シーン遷移
	std::unique_ptr<SceneTransition> sceneTransition_;

private:

	/// <summary>
	/// 使用するモデルデータを読み込む
	/// </summary>
	void LoadModelData();

	/// <summary>
	/// 使用する画像データを読み込む
	/// </summary>
	void LoadSpriteData();

	/// <summary>
	/// 使用するアニメーションデータを読み込む
	/// </summary>
	void LoadAnimationData();

	/// <summary>
	/// 使用する音声データを読み込む
	/// </summary>
	void LoadAudioData();
};
#pragma once
#include"BaseScene.h"

// エンジン機能をインクルード
#include"Camera.h"
#include"DebugCamera.h"
#include"Model.h"
#include"WorldTransform.h"
#include"LightManager.h"
#include"DirectionalLight.h"
#include"Animator.h"

#include"Application/Scene/Transition/Fade.h"

// 前方宣言
class Player;

class GameScene : public BaseScene {
public:

	~GameScene();

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
	void DebugUpdate() override {}

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
	std::string NextSceneName() override { return "Title"; }	

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

	// 太陽の位置を描画するカメラ
	std::unique_ptr<GameEngine::Camera> directionLightCamera_;

	// ライト
	std::unique_ptr<GameEngine::LightManager> lightManager_;
	// 平行光源
	GameEngine::DirectionalLight::DirectionalLightData directionalData_;
	// 点光源
	//GameEngine::PointLight::PointLightData pointLightData_;
	// スポットライト
	//GameEngine::SpotLight::SpotLightData spotLightData_;

	// 地面モデル
	GameEngine::Model* terrainModel_;
	uint32_t grassGH_ = 0u;
	GameEngine::WorldTransform terrainWorldTransform_;

	// スカイボックス
	GameEngine::Model* skyboxModel_;
	GameEngine::WorldTransform skyboxWorldTransform_;
	uint32_t skyboxGH_ = 0u;

	// 平面モデル
	GameEngine::Model* planeModel_;
	uint32_t uvCheckerGH_ = 0u;
	GameEngine::WorldTransform planeWorldTransform_;

	// プレイヤー
	Player* player_;

	// デバック機能をテストする用の変数
	float testNumber = 0;
	Vector3 testVector{};

	// アニメーションモデル
	GameEngine::Model* boneAnimationModel_;
	GameEngine::WorldTransform boneAnimationWorldTransform_;
	// 歩くアニメーションデータ
	std::map<std::string, AnimationData> walkAnimationData_;
	// アニメーションを再生するクラス
	std::unique_ptr<GameEngine::Animator> walkAnimator_;

	// デバック用
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle_;

private:

	/// <summary>
	/// 入力のコマンドを設定する
	/// </summary>
	void InputRegisterCommand();

	/// <summary>
	/// デバックした値を登録
	/// </summary>
	void RegisterDebugParam();

	/// <summary>
	/// デバックした値を取得
	/// </summary>
	void ApplyDebugParam();
};
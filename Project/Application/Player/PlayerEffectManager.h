#pragma once
#include "IGameObject.h"
#include "GameObjectManager.h"
#include "ParticleBehavior.h"
#include "ModelManager.h"
#include "TextureManager.h"

class PlayerEffectManager : public GameEngine::IGameObject {
public:
	PlayerEffectManager(GameEngine::GameObjectManager* objectManager, GameEngine::ModelManager* modelManager, GameEngine::TextureManager* textureManager);
	~PlayerEffectManager() = default;

	// 初期化処理
	//void Initialize() override;
	//
	//// 更新処理
	void Update() override;
	//
	//// 描画処理
	//void Draw() override;

	void StartShockWave(Vector3 pos);

	//void StartDown();
	//void EndDown();

private:
	GameEngine::GameObjectManager* objectManager_ = nullptr;
	
	GameEngine::Model* shockModel_ = nullptr;
	GameEngine::Model* planeXZmodel_ = nullptr;
	GameEngine::Model* planeXYmodel_ = nullptr;

	uint32_t shockGH_ = 0;
	uint32_t dissolveNoiseGH_ = 0;

	uint32_t blastGH_ = 0;

	uint32_t crackGH_ = 0;
	uint32_t dissolveCrackGH_ = 0;

	GameEngine::ParticleBehavior* blastEffect_;
	GameEngine::ParticleBehavior* afterEffect_;

	float timer_ = 0.0f;
	float afterTimer_ = 0.0f;

};
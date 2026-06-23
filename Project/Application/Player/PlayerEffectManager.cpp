#include "PlayerEffectManager.h"
#include "Effect/ShockWave.h"
#include "Effect/ShockFloor.h"
#include "FPSCounter.h"
using namespace GameEngine;

PlayerEffectManager::PlayerEffectManager(GameEngine::GameObjectManager* objectManager, GameEngine::ModelManager* modelManager,GameEngine::TextureManager* textureManager) {

	auto* shockWaveModel = modelManager->GetNameByModel("RushPower.obj");
	shockWaveModel->SetDefaultIsEnableLight(false);
	auto* planeXZModel = modelManager->GetNameByModel("planeXZ.obj");
	planeXZModel->SetDefaultIsEnableLight(false);
	auto* planeXYmodel = modelManager->GetNameByModel("plane.obj");
	planeXYmodel->SetDefaultIsEnableLight(false);

	objectManager_ = objectManager;

	shockModel_ = shockWaveModel;
	planeXZmodel_ = planeXZModel;
	planeXYmodel_ = planeXYmodel;

	crackGH_ = textureManager->GetHandleByName("FX01_Crack_01.png");
	dissolveNoiseGH_ = textureManager->GetHandleByName("noise0.png");
	dissolveCrackGH_ = textureManager->GetHandleByName("FX01_Crack_01_crunch.png");
	shockGH_ = textureManager->GetHandleByName("Power.png");
	blastGH_ = textureManager->GetHandleByName("FX01_Flare_03.png");

	blastEffect_ = objectManager_->AddObject<ParticleBehavior>("HitEffect", 16, textureManager, planeXYmodel, &renderQueue_->GetMainCamera());
	blastEffect_->SetIsLoop(false);

	afterEffect_ = objectManager_->AddObject<ParticleBehavior>("HitAfterEffect", 32, textureManager, planeXYmodel, &renderQueue_->GetMainCamera());
	afterEffect_->SetIsLoop(false);
}

void PlayerEffectManager::StartShockWave(Vector3 pos) {
	// 描画
	objectManager_->AddObject<ShockWave>(shockModel_, planeXYmodel_, blastGH_, shockGH_, dissolveNoiseGH_, pos);

	objectManager_->AddObject<ShockFloor>(planeXZmodel_, crackGH_, dissolveCrackGH_, pos);

	blastEffect_->SetEmitterPos(pos);
	blastEffect_->SetAttractionTarget(pos);
	blastEffect_->SetIsLoop(true);
	timer_ = 0.0f;

	afterEffect_->SetEmitterPos(pos);
	afterEffect_->SetAttractionTarget(pos);
	afterEffect_->SetIsLoop(true);
	afterTimer_ = 0.0f;
}

void PlayerEffectManager::Update() {

	if (timer_ <= 1.0f) {
		timer_ += FpsCounter::deltaTime / 0.2f;
		if (timer_ >= 1.0f) {
			if (blastEffect_->IsLoop()) {
				blastEffect_->SetIsLoop(false);
			}
		}
	}

	if (afterTimer_ <= 1.0f) {
		afterTimer_ += FpsCounter::deltaTime / 0.8f;
		if (afterTimer_ >= 1.0f) {
			if (afterEffect_->IsLoop()) {
				afterEffect_->SetIsLoop(false);
			}
		}
	}
}
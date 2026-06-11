#include "ScenePhase.h"
#include "Application/UI/PlayUIManager.h"
#include "Application/Player/Player.h"
#include "Application/Enemy/BossEnemy.h"

TitlePhase::TitlePhase(PhaseCommonData& commonData) : IScenePhase(commonData) {

}

void TitlePhase::Enter() {

}

void TitlePhase::Update() {

}

void TitlePhase::Exit() {

}

//===========================================
// プレイ
//===========================================

PlayPhase::PlayPhase(PhaseCommonData& commonData, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager) : IScenePhase(commonData) {
	// プレイヤー
	player_ = player;
	// ボス
	bossEnemy_ = bossEnemy;
	// プレイUIを取得
	playUIManager_ = playUIManager;
	playUIManager_->SetActive(false);
}

void PlayPhase::Enter() {
	playUIManager_->SetActive(true);
	// 計測開始
	playTimer_.Reset();
	playTimer_.Start();
}

void PlayPhase::Update() {

	// 計測
	playTimer_.Update();

}

void PlayPhase::Exit() {
	// 計測停止
	playTimer_.Stop();
	commonData_.playTime_ = playTimer_.GetTimer();
	playUIManager_->SetActive(false);
}

//===========================================
// クリア
//===========================================

ClearPhase::ClearPhase(PhaseCommonData& commonData) : IScenePhase(commonData) {

}

void ClearPhase::Enter() {

}

void ClearPhase::Update() {

}

void ClearPhase::Exit() {

}
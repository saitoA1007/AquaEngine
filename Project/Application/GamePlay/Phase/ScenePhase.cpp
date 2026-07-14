#include "ScenePhase.h"
#include "InputCommand.h"
#include "Application/UI/PlayUIManager.h"
#include "Application/Player/Player.h"
#include "Application/Enemy/BossEnemy.h"
#include "Application/Camera/CameraController.h"
#include "Application/Utils/TimeController.h"

TitlePhase::TitlePhase(PhaseCommonData& commonData) : IScenePhase(commonData) {

}

void TitlePhase::Enter() {



}

void TitlePhase::Update() {

}

void TitlePhase::Exit() {

}

//=====================================================
// チュートリアル
//=====================================================

TutorialPhase::TutorialPhase(PhaseCommonData& commonData, CameraController* cameraController, BossEnemy* bossEnemy,PlayUIManager* playUIManager) : IScenePhase(commonData) {

	// カメラ管理を取得
	cameraController_ = cameraController;

	// ボスを取得
	bossEnemy_ = bossEnemy;

	// UIを朱徳
	playUIManager_ = playUIManager;
}

void TutorialPhase::Enter() {

	// UI表示
	playUIManager_->SetIsDrawGamePlayUI(false);
	playUIManager_->SetIsDrawPlayGuide(true);
}

void TutorialPhase::Update() {

	if (bossEnemy_->IsBreakEgg()) {
		playUIManager_->SetIsDrawGamePlayUI(false);
		playUIManager_->SetIsDrawPlayGuide(false);
	}

	// ボスの入りのアニメーションが終わればプレイシーンに移行
	if (BossState::kBattle == bossEnemy_->GetBossState()) {
		commonData_.requestPhase = ScenePhase::kPlay;
	}
}

void TutorialPhase::Exit() {
	// 表示させる
	playUIManager_->SetIsDrawGamePlayUI(true);
	playUIManager_->SetIsDrawPlayGuide(true);
}

//===========================================
// プレイ
//===========================================

PlayPhase::PlayPhase(PhaseCommonData& commonData, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager, CameraController* cameraController) : IScenePhase(commonData) {
	// プレイヤー
	player_ = player;
	// ボス
	bossEnemy_ = bossEnemy;
	// プレイUIを取得
	playUIManager_ = playUIManager;
	// カメラ管理を取得
	cameraController_ = cameraController;
}

void PlayPhase::Enter() {
	// 計測開始
	playTimer_.Reset();
	playTimer_.Start();

	isBarActive_ = false;

	// Hpを設定
	playUIManager_->SetCurrentBossHp(bossEnemy_->GetCurrentHp());
	playUIManager_->SetMaxBossHp(bossEnemy_->GetMaxHp());
	playUIManager_->SetCurrentPlayerHp(player_->GetCurrentHp());
	playUIManager_->SetMaxPlayerHp(player_->GetMaxHp());
}

void PlayPhase::Update() {

	// 黒帯UIを表示
	if (commonData_.inputCommand->IsCommandActive("CameraLockOn")) {
		isBarActive_ = !isBarActive_;
		playUIManager_->SetBarActive(isBarActive_);
	}

	// 現在のHpを設定
	playUIManager_->SetCurrentBossHp(bossEnemy_->GetCurrentHp());
	playUIManager_->SetCurrentPlayerHp(player_->GetCurrentHp());

	// ボスが撃破されればクリアへ移行
	if (bossEnemy_->GetCurrentHp() <= 0) {

	}

	// プレイヤーが撃破されればゲームオーバーへ以降
	if (player_->GetCurrentHp() <= 0) {

	}

	// 計測
	playTimer_.Update();
}

void PlayPhase::Exit() {
	// 計測停止
	playTimer_.Stop();
	commonData_.playTime_ = playTimer_.GetTimer();
	playUIManager_->SetActive(false);
}

//=========================================================
// ポーズ
//=========================================================

PausePhase::PausePhase(PhaseCommonData& commonData) : IScenePhase(commonData) {

}

void PausePhase::Enter() {

}

void PausePhase::Update() {

	// 時間を停止する
	commonData_.timeController_->StartStopTime(3600.0f);


}

void PausePhase::Exit() {
	commonData_.timeController_->Reset();
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
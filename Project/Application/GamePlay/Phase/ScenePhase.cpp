#include "ScenePhase.h"
#include "InputCommand.h"
#include "Application/UI/Managers/TitleUIManager.h"
#include "Application/UI/Managers/PlayUIManager.h"
#include "Application/Player/Player.h"
#include "Application/Enemy/BossEnemy.h"
#include "Application/Camera/CameraController.h"
#include "Application/Utils/TimeController.h"

//=============================================================
// タイトル
//=============================================================

TitlePhase::TitlePhase(PhaseCommonData& commonData, TitleUIManager* titleUIManager) : IScenePhase(commonData) {
	titleUIManager_ = titleUIManager;
}

void TitlePhase::Enter() {
	// UIを有効
	titleUIManager_->SetActive(true);
	// 初期化
	titleUIManager_->Initialize();
}

void TitlePhase::Update() {

	// 決定ボタン
	if (commonData_.inputCommand->IsCommandActive("Decision")) {
		// UIを表示させない
		titleUIManager_->SetIsDraw(false);
	}

	// タイトル文字のフェードが終わればチュートリアルシーンに移行
	if (!titleUIManager_->IsDraw() && !titleUIManager_->IsActiveFadeOut()) {
		commonData_.requestPhase = ScenePhase::kTutorial;
	}
}

void TitlePhase::Exit() {
	// UIを有効
	titleUIManager_->SetActive(false);
}

//=====================================================
// チュートリアル
//=====================================================

TutorialPhase::TutorialPhase(PhaseCommonData& commonData, CameraController* cameraController, BossEnemy* bossEnemy,PlayUIManager* playUIManager) : IScenePhase(commonData) {

	// カメラ管理を取得
	cameraController_ = cameraController;

	// ボスを取得
	bossEnemy_ = bossEnemy;

	// UIを取得
	playUIManager_ = playUIManager;

	// UIを無効
	playUIManager_->SetActive(false);
}

void TutorialPhase::Enter() {

	// UIを有効
	playUIManager_->SetActive(true);

	// UI表示
	playUIManager_->SetIsDrawGamePlayUI(false);
	playUIManager_->SetIsDrawTutorialGuide(true);
	playUIManager_->SetIsDrawPlayGuide(true);
}

void TutorialPhase::Update() {

	if (bossEnemy_->IsBreakEgg()) {
		playUIManager_->SetIsDrawGamePlayUI(false);
		playUIManager_->SetIsDrawTutorialGuide(false);
		playUIManager_->SetIsDrawPlayGuide(false);
	} else {
		// 黒帯UIを表示
		if (commonData_.inputCommand->IsCommandActive("CameraLockOn")) {
			playUIManager_->SetBarActive(cameraController_->UseLetterBoxUI());
		}
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

	// Hpを設定
	playUIManager_->SetCurrentBossHp(bossEnemy_->GetCurrentHp());
	playUIManager_->SetMaxBossHp(bossEnemy_->GetMaxHp());
	playUIManager_->SetCurrentPlayerHp(player_->GetCurrentHp());
	playUIManager_->SetMaxPlayerHp(player_->GetMaxHp());
}

void PlayPhase::Update() {

	// 黒帯UIを表示
	if (commonData_.inputCommand->IsCommandActive("CameraLockOn")) {
		playUIManager_->SetBarActive(cameraController_->UseLetterBoxUI());
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

//=============================================================
// ゲームオーバー
//=============================================================

GameOverPhase::GameOverPhase(PhaseCommonData& commonData) : IScenePhase(commonData) {

}

void GameOverPhase::Enter() {

}

void GameOverPhase::Update() {

}

void GameOverPhase::Exit() {

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
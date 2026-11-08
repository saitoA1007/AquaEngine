#include "pch.h"
#include "ScenePhase.h"
#include "InputCommand.h"
#include "Application/UI/Managers/TitleUIManager.h"
#include "Application/UI/Managers/PlayUIManager.h"
#include "Application/UI/Managers/GameOverUIManager.h"
#include "Application/UI/Managers/ClearUIManager.h"
#include "Application/UI/Managers/PauseUIManager.h"
#include "Application/UI/Managers/EnterMovieUIManager.h"
#include "Application/UI/Managers/OptionUIManager.h"
#include "Application/Player/Player.h"
#include "Application/Enemy/BossEnemy.h"
#include "Application/Camera/CameraController.h"
#include "Application/Utils/TimeController.h"
#include "AudioManager.h"
#include "FPSCounter.h"

//=============================================================
// タイトル
//=============================================================

TitlePhase::TitlePhase(PhaseCommonData& commonData, CameraController* cameraController, TitleUIManager* titleUIManager, Player* player) : IScenePhase(commonData) {

	cameraController_ = cameraController;

	titleUIManager_ = titleUIManager;

	player_ = player;
}

void TitlePhase::Enter() {
	// UIを有効
	titleUIManager_->SetActive(true);
	// 初期化
	titleUIManager_->Initialize();

	// タイトルのカメラに変更
	cameraController_->SetChangeState(CameraState::kTitle);

	// プレイヤーの無効化
	player_->SetActive(false);
}

void TitlePhase::Update() {

	// 決定ボタン
	if (commonData_.inputCommand->IsCommandActive("Decision")) {
		// UIを表示させない
		titleUIManager_->SetIsDraw(false);
	}

	// オプションを開く(ゲーム開始の演出中は開かない)
	if (titleUIManager_->IsDraw() && commonData_.inputCommand->IsCommandActive("OptionOpen")) {
		commonData_.requestPhase = ScenePhase::kOption;
		return;
	}

	// タイトル文字のフェードが終わればチュートリアルシーンに移行
	if (!titleUIManager_->IsDraw() && !titleUIManager_->IsActiveFadeOut()) {
		commonData_.requestPhase = ScenePhase::kTutorial;
	}
}

void TitlePhase::Exit() {
	// UIを無効
	titleUIManager_->SetActive(false);

	// オプションへ行く場合はタイトルの状態を保つ
	if (commonData_.requestPhase == ScenePhase::kOption) { return; }

	// フォローカメラに変更
	cameraController_->SetChangeState(CameraState::kFollow);

	// プレイヤーを有効化
	player_->SetActive(true);
}

//=====================================================
// チュートリアル
//=====================================================

TutorialPhase::TutorialPhase(PhaseCommonData& commonData, CameraController* cameraController, Player* player, BossEnemy* bossEnemy, PlayUIManager* playUIManager,
	EnterMovieUIManager* enterMovieUIManager) : IScenePhase(commonData) {

	// カメラ管理を取得
	cameraController_ = cameraController;

	// ボスを取得
	bossEnemy_ = bossEnemy;

	// プレイヤーを取得
	player_ = player;

	// UIを取得
	playUIManager_ = playUIManager;

	// UIを無効
	playUIManager_->SetActive(false);

	// 入りのムービーのUIを取得
	enterMovieUIManager_ = enterMovieUIManager;
}

void TutorialPhase::Enter() {

	// UIを有効
	playUIManager_->SetActive(true);

	// UI表示
	playUIManager_->SetIsDrawGamePlayUI(false);
	playUIManager_->SetIsDrawTutorialGuide(true);
	playUIManager_->SetIsDrawPlayGuide(true);
	playUIManager_->SetIsDrawArrowUI(true);

	// ポーズから戻った時以外は、ボスの名前を表示し直せるようにする
	if (ScenePhase::kPause != commonData_.GetPrePhase()) {
		isPlayBossName_ = false;
		enterMovieUIManager_->Initialize();
	}
}

void TutorialPhase::Update() {

	if (bossEnemy_->IsBreakEgg()) {
		playUIManager_->SetIsDrawArrowUI(false);
		playUIManager_->SetIsDrawGamePlayUI(false);
		playUIManager_->SetIsDrawTutorialGuide(false);
		playUIManager_->SetIsDrawPlayGuide(false);
		// 演出カメラに変更
		cameraController_->SetChangeState(CameraState::kEnterMovie);
		// 黒帯を表示させる
		playUIManager_->SetBarActive(true);
		// 演出中はロックオンUIを表示しない
		playUIManager_->SetLockOn(false, nullptr);
		// プレイヤーを表示しない
		player_->SetIsDraw(false);

		// ボスが叫び始めたら名前を表示する
		if (!isPlayBossName_ && bossEnemy_->IsScream()) {
			isPlayBossName_ = true;
			enterMovieUIManager_->Play();
		}

		// 入りのムービーを飛ばす
		if (commonData_.inputCommand->IsCommandActive("SkipMovie")) {
			bossEnemy_->SkipEnterMovie();
			// 名前は表示しない
			isPlayBossName_ = true;
			enterMovieUIManager_->Stop();
		}
	} else {
		// 黒帯UIを表示
		playUIManager_->SetBarActive(cameraController_->UseLetterBoxUI());
		// ロックオンUIを更新
		playUIManager_->SetLockOn(cameraController_->IsLockOn(), cameraController_->GetTargetWorld());

		// ポーズ画面を開く
		if (commonData_.inputCommand->IsCommandActive("PauseAction")) {
			commonData_.requestPhase = ScenePhase::kPause;
		}
	}

	// ボスの入りのアニメーションが終わればプレイシーンに移行
	if (BossState::kBattle == bossEnemy_->GetBossState()) {
		commonData_.requestPhase = ScenePhase::kPlay;
	}
}

void TutorialPhase::Exit() {
	// 表示させる
	if (commonData_.requestPhase != ScenePhase::kPause) {
		playUIManager_->SetIsDrawGamePlayUI(true);
		playUIManager_->SetIsDrawPlayGuide(true);
		playUIManager_->SetIsDrawArrowUI(false);
		// プレイヤーを表示
		player_->SetIsDraw(true);
		// ボスの名前を消す
		enterMovieUIManager_->Stop();
		// フォローカメラに変更
		cameraController_->SetChangeState(CameraState::kFollow);
	}
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

	if (ScenePhase::kPause != commonData_.GetPrePhase()) {
		// リセット
		playTimer_.Reset();
	}
	// 計測開始
	playTimer_.Start();

	// Hpを設定
	playUIManager_->SetCurrentBossHp(bossEnemy_->GetCurrentHp());
	playUIManager_->SetMaxBossHp(bossEnemy_->GetMaxHp());
	playUIManager_->SetCurrentPlayerHp(player_->GetCurrentHp());
	playUIManager_->SetMaxPlayerHp(player_->GetMaxHp());
}

void PlayPhase::Update() {

	// ポーズ画面を開く
	if (commonData_.inputCommand->IsCommandActive("PauseAction")) {
		commonData_.requestPhase = ScenePhase::kPause;
	}

	// 黒帯UIを表示
	playUIManager_->SetBarActive(cameraController_->UseLetterBoxUI());
	// ロックオンUIを更新
	playUIManager_->SetLockOn(cameraController_->IsLockOn(), cameraController_->GetTargetWorld());

	// 現在のHpを設定
	playUIManager_->SetCurrentBossHp(bossEnemy_->GetCurrentHp());
	playUIManager_->SetCurrentPlayerHp(player_->GetCurrentHp());

	// ボスが撃破されればクリアへ移行
	if (bossEnemy_->GetCurrentHp() <= 0) {
		commonData_.requestPhase = ScenePhase::kClear;
	}

	// プレイヤーが撃破されればゲームオーバーへ移行
	if (player_->GetCurrentHp() <= 0) {
		commonData_.requestPhase = ScenePhase::kGameOver;
	}

	// 計測
	playTimer_.Update();
}

void PlayPhase::Exit() {
	if (commonData_.requestPhase != ScenePhase::kPause) {
		// 計測停止
		playTimer_.Stop();
		commonData_.playTime_ = playTimer_.GetTimer();

		// ロックオンUIを消す
		playUIManager_->SetLockOn(false, nullptr);
	}
}

//=========================================================
// ポーズ
//=========================================================

PausePhase::PausePhase(PhaseCommonData& commonData, PauseUIManager* pauseUIManager) : IScenePhase(commonData) {
	pauseUIManager_ = pauseUIManager;
	pauseUIManager_->SetActive(false);
}

void PausePhase::Enter() {
	// オプションから戻ってきた時以外は、戻り先のフェーズを保存する
	if (ScenePhase::kOption != commonData_.GetPrePhase()) {
		returnPhase_ = commonData_.GetPrePhase();
		selectNum_ = 0;
		pauseUIManager_->SetType(PauseUIManager::SelectType::kBack);
	}

	pauseUIManager_->SetActive(true);
	pauseUIManager_->Initialize();
	// 入りのアニメーション
	pauseUIManager_->Play(PauseUIManager::Phase::kIn);
}

void PausePhase::Update() {

	// 時間を停止する
	commonData_.timeController_->StartStopTime(3600.0f);

	if (!pauseUIManager_->IsAnimation()) {
		// 操作
		if (commonData_.inputCommand->IsCommandActive("SelectUp")) {

			if (selectNum_ >= 1) {
				selectNum_--;
			}
			pauseUIManager_->SetType(static_cast<PauseUIManager::SelectType>(selectNum_));
		}

		if (commonData_.inputCommand->IsCommandActive("SelectDown")) {

			if (selectNum_ < static_cast<int32_t>(PauseUIManager::SelectType::kMaxCount) - 1) {
				selectNum_++;
			}
			pauseUIManager_->SetType(static_cast<PauseUIManager::SelectType>(selectNum_));
		}

		// 決定
		if (commonData_.inputCommand->IsCommandActive("Decision")) {

			if (PauseUIManager::SelectType::kRetry == pauseUIManager_->GetType()) {
				// やり直す
				commonData_.requestPhase = ScenePhase::kTutorial;
			} else if (PauseUIManager::SelectType::kOption == pauseUIManager_->GetType()) {
				// オプションを開く
				commonData_.requestPhase = ScenePhase::kOption;
			} else if (PauseUIManager::SelectType::kBackTitle == pauseUIManager_->GetType()) {
				// タイトルへ戻る
				commonData_.requestPhase = ScenePhase::kTitle;
				// リセットする
				commonData_.resetScene = true;
			} else {

				// 戻る場合はアニメーション
				pauseUIManager_->Play(PauseUIManager::Phase::kOut);
			}
		}

		// 戻る
		if (commonData_.inputCommand->IsCommandActive("PauseAction")) {
			// 戻る場合はアニメーション
			pauseUIManager_->Play(PauseUIManager::Phase::kOut);
		}
	}

	// アニメーションが終了したら遷移する
	if (PauseUIManager::SelectType::kBack == pauseUIManager_->GetType()) {
		if (PauseUIManager::Phase::kOut == pauseUIManager_->GetPhase()) {
			if (!pauseUIManager_->IsAnimation()) {

				if (PauseUIManager::SelectType::kBack == pauseUIManager_->GetType()) {
					// ポーズを開く前のフェーズへ戻る
					commonData_.requestPhase = returnPhase_;
				}
			}
		}
	}
}

void PausePhase::Exit() {
	// オプションへ行く場合は時間を止めたままにする
	if (commonData_.requestPhase != ScenePhase::kOption) {
		commonData_.timeController_->Reset();
	}
	pauseUIManager_->SetActive(false);
}

//=============================================================
// ゲームオーバー
//=============================================================

GameOverPhase::GameOverPhase(PhaseCommonData& commonData, GameOverUIManager* gameOverUIManager) : IScenePhase(commonData) {

	gameOverUIManager_ = gameOverUIManager;
	gameOverUIManager_->SetActive(false);
}

void GameOverPhase::Enter() {
	// UIを有効化
	gameOverUIManager_->SetActive(true);
	// 初期化
	gameOverUIManager_->Initialize();
	gameOverUIManager_->StartEnterAnimation();
}

void GameOverPhase::Update() {

	// 操作
	if (commonData_.inputCommand->IsCommandActive("SelectUp")) {
		gameOverUIManager_->SetType(GameOverUIManager::SelectType::kRetry);
	}

	if (commonData_.inputCommand->IsCommandActive("SelectDown")) {
		gameOverUIManager_->SetType(GameOverUIManager::SelectType::kBackTitle);
	}

	// 決定
	if (commonData_.inputCommand->IsCommandActive("Decision")) {
		gameOverUIManager_->Play();

		// リセットする
		commonData_.resetScene = true;

		// 遷移させる
		if (gameOverUIManager_->GetType() == GameOverUIManager::SelectType::kRetry) {
			// やり直す
			commonData_.requestPhase = ScenePhase::kTutorial;
		} else {
			// タイトルへ戻る
			commonData_.requestPhase = ScenePhase::kTitle;
		}
	}
}

void GameOverPhase::Exit() {
	// UIを無効化
	gameOverUIManager_->SetActive(false);
}

//===========================================
// クリア
//===========================================

ClearPhase::ClearPhase(PhaseCommonData& commonData, ClearUIManager* clearUIManager) : IScenePhase(commonData) {

	clearUIManager_ = clearUIManager;
	clearUIManager_->SetActive(false);
}

void ClearPhase::Enter() {
	// UIを有効化
	clearUIManager_->SetActive(true);
	// 初期化
	clearUIManager_->Initialize();
	// アニメーション開始
	clearUIManager_->StartEnterAnimation();

	// プレイ時間を取得
	clearUIManager_->SetTime(commonData_.playTime_);
}

void ClearPhase::Update() {

	// 決定
	if (commonData_.inputCommand->IsCommandActive("Decision")) {
		// リセットする
		commonData_.resetScene = true;
		// タイトルへ戻る
		commonData_.requestPhase = ScenePhase::kTitle;
	}
}

void ClearPhase::Exit() {
	// UIを無効化
	clearUIManager_->SetActive(false);
}

//===========================================
// オプション
//===========================================

OptionPhase::OptionPhase(PhaseCommonData& commonData, OptionUIManager* optionUIManager, CameraController* cameraController) : IScenePhase(commonData) {

	optionUIManager_ = optionUIManager;
	optionUIManager_->SetActive(false);

	cameraController_ = cameraController;

	// SEの音量確認用の音
	seCheckHandle_ = GameEngine::AudioManager::GetInstance().GetHandleByName("se.mp3");

	// 初期設定を反映
	ApplySettings();
}

void OptionPhase::Enter() {
	// 戻り先を保存
	returnPhase_ = commonData_.GetPrePhase();

	selectNum_ = 0;
	upInput_ = {};
	downInput_ = {};
	leftInput_ = {};
	rightInput_ = {};

	// UIを有効化
	optionUIManager_->SetActive(true);
	optionUIManager_->Initialize();
	ApplySettings();
}

void OptionPhase::Update() {

	// ポーズから開いた場合は時間を止めたままにする
	if (ScenePhase::kPause == returnPhase_) {
		commonData_.timeController_->StartStopTime(3600.0f);
	}

	constexpr int32_t kItemCount = static_cast<int32_t>(static_cast<uint32_t>(OptionUIManager::Item::kMaxCount));

	// 項目の選択
	if (IsRepeatInput("OptionUp", upInput_)) {
		selectNum_ = (selectNum_ + kItemCount - 1) % kItemCount;
	}
	if (IsRepeatInput("OptionDown", downInput_)) {
		selectNum_ = (selectNum_ + 1) % kItemCount;
	}
	optionUIManager_->SetSelectItem(static_cast<OptionUIManager::Item>(selectNum_));

	// 値の変更
	if (IsRepeatInput("OptionLeft", leftInput_)) {
		ChangeValue(-1);
	}
	if (IsRepeatInput("OptionRight", rightInput_)) {
		ChangeValue(1);
	}

	// 決定
	bool isBack = false;
	if (commonData_.inputCommand->IsCommandActive("Decision")) {
		if (OptionUIManager::Item::kBack == optionUIManager_->GetSelectItem()) {
			isBack = true;
		} else if (selectNum_ >= static_cast<int32_t>(OptionUIManager::Item::kCameraX)) {
			// 反転設定は決定でも切り替えられる
			ChangeValue(1);
		}
	}

	// 戻る
	if (commonData_.inputCommand->IsCommandActive("Cancel") ||
		commonData_.inputCommand->IsCommandActive("OptionOpen") ||
		commonData_.inputCommand->IsCommandActive("PauseAction")) {
		isBack = true;
	}

	if (isBack) {
		commonData_.requestPhase = returnPhase_;
	}
}

void OptionPhase::Exit() {
	// UIを無効化
	optionUIManager_->SetActive(false);

	// ポーズ以外に戻る場合は時間を戻す
	if (ScenePhase::kPause != returnPhase_) {
		commonData_.timeController_->Reset();
	}
}

bool OptionPhase::IsRepeatInput(const std::string& commandName, RepeatInput& state) {
	bool isPush = commonData_.inputCommand->IsCommandActive(commandName);

	bool result = false;
	if (isPush) {
		if (!state.isPrePush) {
			// 押した瞬間
			result = true;
			state.timer = kRepeatDelay_;
		} else {
			// 押し続けている間は一定間隔で受け付ける
			state.timer -= GameEngine::FpsCounter::GetDeltaTime();
			if (state.timer <= 0.0f) {
				result = true;
				state.timer += kRepeatInterval_;
			}
		}
	}
	state.isPrePush = isPush;
	return result;
}

void OptionPhase::ChangeValue(int32_t dir) {

	switch (static_cast<OptionUIManager::Item>(selectNum_)) {
	case OptionUIManager::Item::kBgmVolume:
		settings_.bgmVolumeLevel = std::clamp(settings_.bgmVolumeLevel + dir, 0, OptionSettings::kMaxVolumeLevel);
		break;

	case OptionUIManager::Item::kSeVolume:
	{
		int32_t pre = settings_.seVolumeLevel;
		settings_.seVolumeLevel = std::clamp(settings_.seVolumeLevel + dir, 0, OptionSettings::kMaxVolumeLevel);
		if (pre != settings_.seVolumeLevel) {
			ApplySettings();
			// 変更後の音量を確認できるように鳴らす
			GameEngine::AudioManager::GetInstance().Play(seCheckHandle_, 1.0f, false, "SE");
		}
		break;
	}

	case OptionUIManager::Item::kCameraX:
		settings_.isInvertCameraX = !settings_.isInvertCameraX;
		break;

	case OptionUIManager::Item::kCameraY:
		settings_.isInvertCameraY = !settings_.isInvertCameraY;
		break;

	default:
		break;
	}

	ApplySettings();
}

void OptionPhase::ApplySettings() {
	// 音量
	auto& audio = GameEngine::AudioManager::GetInstance();
	audio.SetGroupVolume("BGM", OptionSettings::ToVolume(settings_.bgmVolumeLevel));
	audio.SetGroupVolume("SE", OptionSettings::ToVolume(settings_.seVolumeLevel));

	// 視点操作
	cameraController_->SetInvertX(settings_.isInvertCameraX);
	cameraController_->SetInvertY(settings_.isInvertCameraY);

	// UI
	optionUIManager_->SetSettings(settings_);
}

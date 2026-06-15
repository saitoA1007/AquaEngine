#include "BossStateIn.h"

BossStateIn::BossStateIn(BossStateCommonData& commonData) : stateCommonData_(commonData) {

}

void BossStateIn::Enter() {
	// 初期位置
	stateCommonData_.worldTransform->transform_.translate = { 0.0f,8.0f,0.0f };
	stateCommonData_.worldTransform->transform_.rotate = { 0.0f,3.2f,0.0f };

	
}

void BossStateIn::Update() {

	stateCommonData_.bossStateRequest = BossState::kBattle;
}

void BossStateIn::Exit() {

}
#include "BossStateIn.h"

BossStateIn::BossStateIn(BossStateCommonData& commonData) : stateCommonData_(commonData) {

}

void BossStateIn::Enter() {

}

void BossStateIn::Update() {
	stateCommonData_.bossStateRequest = BossState::kBattle;
}

void BossStateIn::Exit() {

}
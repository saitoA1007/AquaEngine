#include "HpBarUI.h"

using namespace GameEngine;

HpBarUI::HpBarUI() {

	// ペアレント
	barSprite_.SetParent(&baseWorld_);
	effectSprite_.SetParent(&baseWorld_);
	frameSprite_.SetParent(&baseWorld_);
}

void HpBarUI::Initialize() {

}

void HpBarUI::Update() {


	barSprite_.Update();
	effectSprite_.Update();
	frameSprite_.Update();
}

void HpBarUI::Draw() {

}
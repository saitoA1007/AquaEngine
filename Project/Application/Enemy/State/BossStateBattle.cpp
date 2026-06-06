#include "BossStateBattle.h"
#include "RandomGenerator.h"
#include "LogManager.h"
#include "Application/Enemy/BattleAction/BossBattleAction.h"

using namespace GameEngine;

BossStateBattle::BossStateBattle(BossStateCommonData& commonData, Vector3* playerPos) : stateCommonData_(commonData) {

	// プレイヤーの位置を取得
	battleStateCommonData_.playerPos = playerPos;

	// 各行動を登録する
	battleStatesTable_[static_cast<size_t>(BossBattleState::kRushAttack)] = std::make_unique<BossRushAttackAction>(battleStateCommonData_);
	battleStatesTable_[static_cast<size_t>(BossBattleState::kWait)] = std::make_unique<BossWaitAction>(battleStateCommonData_);

	// 初期化
	currentBattleState_ = BossBattleState::kWait;
	battleStatesTable_[static_cast<size_t>(currentBattleState_)]->Initialize();

	// 行動に重み付け
	lotteryList_ = {
		{ BossBattleState::kRushAttack,10 }, // 突進攻撃
		{ BossBattleState::kWait,10 }, // 待機
	};

	// 値を登録
	//for (auto& state : battleStatesTable_) {
	//	state->RegisterParameter(stateCommonData_.debugParame);
	//}
}

void BossStateBattle::Enter() {
	currentBattleState_ = BossBattleState::kWait;
	battleStatesTable_[static_cast<size_t>(currentBattleState_)]->Initialize();
}

void BossStateBattle::Update() {
	// 切り替え処理
	if (battleStatesTable_[static_cast<size_t>(currentBattleState_)]->IsFinished()) {
		// 終了処理をおこなう
		battleStatesTable_[static_cast<size_t>(currentBattleState_)]->Finalize();
		// 次の行動を取得する
		currentBattleState_ = SelectWeightedBattleState();
		Log("BossEnemy change battleState");
		// 初期化する
		battleStatesTable_[static_cast<size_t>(currentBattleState_)]->Initialize();
	}

	// 指定した状態による更新処理
	battleStatesTable_[static_cast<size_t>(currentBattleState_)]->Update();

	// 更新
	stateCommonData_.worldTransform->transform_ = battleStateCommonData_.transform;
}

void BossStateBattle::Exit() {

}

BossBattleState BossStateBattle::SelectWeightedBattleState() {
	BossBattleState result = BossBattleState::kWait;

	// 全体の重みを計算する
	int32_t totalWeight = 0;
	for (const auto& item : lotteryList_) {
		totalWeight += item.weight;
	}

	uint32_t randomValue = RandomGenerator::Get(int32_t(0), int32_t(totalWeight - 1));

	for (const auto& item : lotteryList_) {
		if (randomValue < item.weight) {
			result = item.behavior;
			break;
		}
		// 次の範囲へ進むために値を引く
		randomValue -= item.weight;
	}

	return result;
}
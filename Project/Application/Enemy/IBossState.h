#pragma once
#include <optional>
#include "WorldTransform.h"
#include "DebugParameter.h"

// ボスの基本的な状態
enum class BossState {
	kIn,     // 入りの状態
	kBattle, // 戦いの状態
	kOut,    // 終わりの状態

	kMaxCount // 数
};

// 状態で共有するデータ
struct BossStateCommonData {
	// ワールド行列
	GameEngine::WorldTransform* worldTransform = nullptr;

	// 状態の切り替えを管理
	std::optional<BossState> bossStateRequest = std::nullopt;

	// パラメータの保存用
	GameEngine::DebugParameter* debugParame;
};

class IBossState {
public:
	virtual ~IBossState() = default;
	virtual void Enter() = 0;
	virtual void Update() = 0;
	virtual void Exit() = 0;
};
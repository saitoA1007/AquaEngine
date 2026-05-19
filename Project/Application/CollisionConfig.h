#pragma once
#include <cstdint>

// プレイヤー陣営
static inline const uint32_t kCollisionAttributePlayer = 0b1;
// 敵陣営
static inline const uint32_t kCollisionAttributeEnemy = 0b1 << 1;
// 地形陣営
static inline const uint32_t kCollisionAttributeTerrain = 0b1 << 2;

/// <summary>
/// 当たり判定がもつID
/// </summary>
enum class CollisionTypeID : uint32_t {
	Default, // 通常
	Player,  // プレイヤー
	Boss,    // ボス
	Wall,    // 壁
	Ground,  // 地面
	IceFall, // つらら
	Wind,    // ボスの風攻撃
	Heart,   // 回復
	BoundaryWall, // 移動範囲制限用の壁
};

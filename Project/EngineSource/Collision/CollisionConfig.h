#pragma once
#include<cstdint>

namespace GameEngine {

	// プレイヤー陣営
	const uint32_t kCollisionAttributePlayer = 0b1;
	// 敵陣営
	const uint32_t kCollisionAttributeEnemy = 0b1 << 1;
	// 地形陣営
	const uint32_t kCollisionAttributeTerrain = 0b1 << 2;
}

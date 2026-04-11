#pragma once
#include <variant>
#include "Geometry.h"

namespace GameEngine {

	// 当たり判定の形状
	struct CollisionType {
		// 当たり判定の属性
		std::variant<Sphere, AABB, OBB, Segment> type;

		// 形状を取得する
		template<typename T>
		const T* Get() const { return std::get_if<T>(&type); }
		template<typename T>
		T* Get() { return std::get_if<T>(&type); }
	};
}
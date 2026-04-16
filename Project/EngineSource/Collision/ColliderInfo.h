#pragma once
#include <variant>
#include "Geometry.h"

namespace GameEngine {

	// 形状
	enum ShapeType {
		kSphere,
		kAABB,
		kOBB,
		kSegment,

		kMaxCount,
	};

	// 当たり判定の形状
	struct CollisionData {
		// 当たり判定の属性
		std::variant<Sphere, AABB, OBB, Segment> data;
		// 形状タイプ
		ShapeType shapeType;

		// 形状を取得する
		template<typename T>
		const T* Get() const { return std::get_if<T>(&data); }
		template<typename T>
		T* Get() { return std::get_if<T>(&data); }
	};
}
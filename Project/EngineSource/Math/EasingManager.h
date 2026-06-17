#pragma once
#include"Vector2.h"
#include"Vector3.h"
#include"Quaternion.h"

namespace GameEngine {
	
	/// <summary>
	/// 線形補間
	/// </summary>
	/// <param name="start">始点</param>
	/// <param name="end">終点</param>
	/// <param name="timer">時間</param>
	/// <returns></returns>
	float Lerp(const float& start, const float& end, const float& t);
	Vector2 Lerp(Vector2 start, Vector2 end, float t);
	Vector3 Lerp(const Vector3& start, const Vector3& end, const float& t);
	Quaternion Lerp(const Quaternion& start, const Quaternion& end, const float& t);

	// イージングイン
	float EaseIn(float t);

	// イージングアウト
	float EaseOut(float t);

	// イージングインアウト
	float EaseInOut(float t);

	// 球面線形補間
	Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t);
	Vector3 Slerp(const Vector3& start, const Vector3& end, float t);
}


#include"EasingManager.h"
#include<cmath>
#include <algorithm>
#include"EngineSource/Math/MyMath.h"

float Lerp(const float& start, const float& end, const float& t) {
	//return (1.0f - t) * start + t * end;
	return start + t * (end - start);
}

Vector3 Lerp(const Vector3& start, const Vector3& end, const float& t) {
	return Vector3(start.x + t * (end.x - start.x), start.y + t * (end.y - start.y) ,start.z + t * (end.z - start.z));
}

Quaternion Lerp(const Quaternion& start, const Quaternion& end, const float& t) {
	return Quaternion(start.x + t * (end.x - start.x), start.y + t * (end.y - start.y), start.z + t * (end.z - start.z), start.w + t * (end.w - start.w));
}

float EaseIn(const float& t) {
	return t * t;
}

float EaseOut(const float& t) {
	return 1.0f - std::powf(1.0f - t, 2.0f);
}

float EaseInOut(const float& t) {
	if (t < 0.5f) {
		return 2.0f * t * t;
	} else {
		return 1.0f - std::powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
	}
}

Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t) {
	Quaternion q1Copy = q1;
	float dot = Dot(q0, q1);
	// 四元数の符号が逆だと最短経路で補間されないので反転
	if (dot < 0.0f) {
		dot = -dot;
		q1Copy = -q1;
	}
	// 内積が1,0fを超えないようにする
	dot = std::clamp(dot, -1.0f, 1.0f);

	if (dot > 0.9999f) {
		Quaternion result = Lerp(q0, q1Copy, t);
		return Normalize(result);
	}

	// なす角
	float theta = std::acosf(dot);
	float sinTheta = std::sinf(theta);
	// 補間係数を求める
	float scale0 = std::sinf((1.0f - t) * theta) / sinTheta;
	float scale1 = std::sinf(t * theta) / sinTheta;
	return scale0 * q0 + scale1 * q1Copy;
}
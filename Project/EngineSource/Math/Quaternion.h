#pragma once

struct Quaternion {
	float x;
	float y;
	float z;
	float w;

	Quaternion operator+(const Quaternion& other) { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
	Quaternion operator*(const float& other) { return { x * other, y * other, z * other, w * other }; }
	friend Quaternion operator*(float other, const Quaternion& q) { return { q.x * other, q.y * other, q.z * other, q.w * other }; }
	Quaternion operator-() const { return { -x, -y, -z,-w }; }
};
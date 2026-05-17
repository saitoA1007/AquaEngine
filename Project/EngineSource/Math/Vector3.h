#pragma once
#include <cmath>

struct Vector3 {
	float x, y, z;

	Vector3 operator+(const Vector3& other) const { return { x + other.x, y + other.y, z + other.z }; }
	Vector3 operator-(const Vector3& other) const { return { x - other.x, y - other.y, z - other.z }; }
	Vector3 operator*(const Vector3& other) const { return { x * other.x, y * other.y, z * other.z }; }
	Vector3 operator/(const Vector3& other) const { return { x / other.x, y / other.y, z / other.z }; }
	Vector3 operator+=(const Vector3& other) { return { x += other.x, y += other.y, z += other.z }; }
	Vector3 operator-=(const Vector3& other) { return { x -= other.x, y -= other.y, z -= other.z }; }
	Vector3 operator*=(const Vector3& other) { return { x *= other.x, y *= other.y, z *= other.z }; }
	Vector3 operator/=(const Vector3& other) { return { x /= other.x, y /= other.y, z /= other.z }; }
	Vector3 operator+(const float& other) const { return { x + other, y + other, z + other }; }
	Vector3 operator-(const float& other) const { return { x - other, y - other, z - other }; }
	Vector3 operator*(const float& other) const { return { x * other, y * other, z * other }; }
	Vector3 operator/(const float& other) const { return { x / other, y / other, z / other }; }
	
	// ベクトルの長さ
	float Length() const {
		return std::sqrt(x * x + y * y + z * z);
	}

	// ベクトルの長さの2乗
	float LengthSquared() const {
		return x * x + y * y + z * z;
	}

	// 正規化
	void Normalize() {
		float len = Length();
		// ゼロ除算を防ぐためのチェック
		if (len > 0.0f) {
			x /= len;
			y /= len;
			z /= len;
		}
	}
};
#pragma once
#include <cmath>

struct Vector2 {
	float x, y;

	Vector2 operator+(const Vector2& other) const { return { x + other.x, y + other.y }; }
	Vector2 operator-(const Vector2& other) const { return { x - other.x, y - other.y }; }
	Vector2 operator*(const Vector2& other) const { return { x * other.x, y * other.y }; }
	Vector2 operator/(const Vector2& other) const { return { x / other.x, y / other.y }; }
	Vector2 operator+=(const Vector2& other) { return { x += other.x, y += other.y }; }
	Vector2 operator-=(const Vector2& other) { return { x -= other.x, y -= other.y }; }
	Vector2 operator*=(const Vector2& other) { return { x *= other.x, y *= other.y }; }
	Vector2 operator/=(const Vector2& other) { return { x /= other.x, y /= other.y }; }
	Vector2 operator+(const float& other) const { return { x + other, y + other }; }
	Vector2 operator-(const float& other) const { return { x - other, y - other }; }
	Vector2 operator*(const float& other) const { return { x * other, y * other }; }
	Vector2 operator/(const float& other) const { return { x / other, y / other }; }

	// ベクトルの長さ
	float Length() const {
		return std::sqrt(x * x + y * y);
	}

	// ベクトルの長さの2乗
	float LengthSquared() const {
		return x * x + y * y;
	}

	// 正規化
	void Normalize() {
		float len = Length();
		// ゼロ除算を防ぐためのチェック
		if (len > 0.0f) {
			x /= len;
			y /= len;
		}
	}

	// ベクトルの距離を求める
	static float GetDistance(Vector2 v1, Vector2 v2) {
		return std::powf(v1.x - v2.x, 2) + std::powf(v1.y - v2.y, 2);
	}
};

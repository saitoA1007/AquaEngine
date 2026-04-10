#pragma once

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
};

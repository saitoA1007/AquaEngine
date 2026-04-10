#pragma once

struct Vector4 {
	float x, y, z, w;

	Vector4 operator+(const Vector4& other) const { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
	Vector4 operator-(const Vector4& other) const { return { x - other.x, y - other.y, z - other.z, w - other.w }; }
	Vector4 operator*(const Vector4& other) const { return { x * other.x, y * other.y, z * other.z, w * other.w }; }
	Vector4 operator/(const Vector4& other) const { return { x / other.x, y / other.y, z / other.z, w / other.w }; }
	Vector4 operator+=(const Vector4& other) { return { x += other.x, y += other.y, z += other.z, w += other.w }; }
	Vector4 operator-=(const Vector4& other) { return { x -= other.x, y -= other.y, z -= other.z, w -= other.w }; }
	Vector4 operator*=(const Vector4& other) { return { x *= other.x, y *= other.y, z *= other.z, w *= other.w }; }
	Vector4 operator/=(const Vector4& other) { return { x /= other.x, y /= other.y, z /= other.z, w /= other.w }; }
	Vector4 operator+(const float& other) const { return { x + other, y + other, z + other, w + other }; }
	Vector4 operator-(const float& other) const { return { x - other, y - other, z - other, w - other }; }
	Vector4 operator*(const float& other) const { return { x * other, y * other, z * other, w * other }; }
	Vector4 operator/(const float& other) const { return { x / other, y / other, z / other, w / other }; }
};

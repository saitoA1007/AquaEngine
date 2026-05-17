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

	// ベクトルの長さ
	float Norm() const {
		return std::sqrt(x * x + y * y + z * z + w * w);
	}

	float NormSquared() const {
		return x * x + y * y + z * z + w * w;
	}

	// 正規化
	void Normalize() {
		float norm = Norm();
		// ゼロ除算を防ぐためのチェック
		if (norm == 0.0f) {
			Identity();
		} else {
			x /= norm;
			y /= norm;
			z /= norm;
			w /= norm;
		}
	}

	void Conjugate() {
		x *= 1.0f;
		y *= 1.0f;
		z *= 1.0f;
	}

	static Quaternion Identity() {
		return { 0.0f, 0.0f, 0.0f, 1.0f };
	}
};
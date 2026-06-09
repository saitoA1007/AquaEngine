#include "MyMath.h"
#include <cassert>
#include <cmath>
#include <algorithm>

namespace GameEngine {
	namespace Math {

		Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs) {

			Quaternion result;
			result.w = lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z;
			result.x = lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y;
			result.y = lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x;
			result.z = lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w;
			return result;
		}

		Quaternion Conjugate(const Quaternion& quaternion) {
			return { -quaternion.x, -quaternion.y, -quaternion.z, quaternion.w };
		}

		float Norm(const Quaternion& quaternion) {
			return std::sqrt(quaternion.x * quaternion.x + quaternion.y * quaternion.y + quaternion.z * quaternion.z + quaternion.w * quaternion.w);
		}

		Quaternion Normalize(const Quaternion& quaternion) {
			float norm = Math::Norm(quaternion);
			// 0除算を避けるため単位Quaternionを返す
			if (norm == 0.0f) {
				return Quaternion::Identity();
			}
			return { quaternion.x / norm, quaternion.y / norm, quaternion.z / norm, quaternion.w / norm };
		}

		Quaternion Inverse(const Quaternion& quaternion) {
			float norm = Math::Norm(quaternion);
			// 0除算を避けるため単位Quaternionを返す
			if (norm == 0.0f) {
				return Quaternion::Identity();
			}
			Quaternion conjugate = Math::Conjugate(quaternion);
			float invNorm = 1.0f / (norm * norm);
			return { conjugate.x * invNorm, conjugate.y * invNorm,conjugate.z * invNorm,conjugate.w * invNorm };
		}

		Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle) {
			float halfAngle = angle / 2.0f;
			float sin = std::sin(halfAngle);
			float cos = std::cos(halfAngle);
			return { axis.x * sin, axis.y * sin, axis.z * sin, cos };
		}

		Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion) {
			// 四元数とベクトルの回転（q * v * q^-1）
			Quaternion r = { vector.x, vector.y, vector.z,0.0f };
			// quaternionの共役を求める
			Quaternion qConj = Math::Conjugate(quaternion);
			Quaternion rotated = Math::Multiply(Math::Multiply(quaternion, r), qConj);
			return { rotated.x, rotated.y, rotated.z };
		}

		Matrix4x4 MakeRotateMatrix(const Quaternion& q) {
			Matrix4x4 result = {
				q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z, 2.0f * (q.x * q.y + q.w * q.z), 2.0f * (q.x * q.z - q.w * q.y), 0.0f,
				2.0f * (q.x * q.y - q.w * q.z), q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z, 2.0f * (q.y * q.z + q.w * q.x), 0.0f,
				2.0f * (q.x * q.z + q.w * q.y), 2.0f * (q.y * q.z - q.w * q.x), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z, 0.0f,
				0.0f,0.0f,0.0f,1.0f,
			};
			return result;
		}

		float Dot(const Quaternion& a, const Quaternion& b) {
			return  a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
		}

		Matrix4x4 MakeRotateAxisAngle(const Vector3& axis, float angle) {

			float cos = std::cosf(angle);
			float sin = std::sinf(angle);
			float t = 1.0f - cos;

			Matrix4x4 result = {
				axis.x * axis.x * t + cos, axis.x * axis.y * t + axis.z * sin, axis.x * axis.z * t - axis.y * sin, 0.0f,
				axis.x * axis.y * t - axis.z * sin, axis.y * axis.y * t + cos, axis.y * axis.z * t + axis.x * sin, 0.0f,
				axis.x * axis.z * t + axis.y * sin, axis.y * axis.z * t - axis.x * sin, axis.z * axis.z * t + cos, 0.0f,
				0.0f,0.0f,0.0f,1.0f,
			};
			return result;
		}

		Quaternion MakeEulerQuaternion(float pitch, float yaw, float roll) {
			// pitch==x, yaw==y, roll==z
			Quaternion qx = Math::MakeRotateAxisAngleQuaternion({ 1,0,0 }, pitch);
			Quaternion qy = Math::MakeRotateAxisAngleQuaternion({ 0,1,0 }, yaw);
			Quaternion qz = Math::MakeRotateAxisAngleQuaternion({ 0,0,1 }, roll);

			// 回転順序ZYX
			return Math::Multiply(qy, Math::Multiply(qx, qz));
		}

		Matrix4x4 MakeWorldMatrixFromEulerRotation(const Vector3 position, const Vector3& rotateEuler, const Vector3& scale) {

			// 回転行列を作成
			Quaternion rotate = Math::MakeEulerQuaternion(rotateEuler.x, rotateEuler.y, rotateEuler.z);
			Matrix4x4 rotateMatrix = Math::MakeRotateMatrix(rotate);

			// 拡縮行列
			Matrix4x4 scaleMatrix = {
				scale.x, 0.0f,   0.0f,   0.0f,
				0.0f,   scale.y, 0.0f,   0.0f,
				0.0f,   0.0f,   scale.z, 0.0f,
				0.0f,   0.0f,   0.0f,    1.0f
			};

			// 平行移動行列
			Matrix4x4 translateMatrix = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				position.x, position.y, position.z, 1.0f
			};

			// SRT行列
			Matrix4x4 worldMatrix = (scaleMatrix * rotateMatrix) * translateMatrix;
			return worldMatrix;
		}

		Quaternion DirectionToQuaternion(const Vector3& direction, const Vector3& up) {
			// 基準となる前方向ベクトル
			const Vector3 kForward = { 0.0f, 0.0f, 1.0f };

			// 方向ベクトルを正規化
			float len = direction.Length();
			// 0ベクトル確認
			if (len < 1e-6f) {
				return Quaternion::Identity();
			}
			Vector3 dir = {
				direction.x / len,
				direction.y / len,
				direction.z / len
			};

			float dot = Math::Dot(kForward, dir);

			// 既に同じ方向を向いている場合単位Quaternionを返す
			if (dot >= 1.0f - 1e-6f) {
				return Quaternion::Identity();
			}

			// ほぼ逆方向の場合upベクトルを軸に180度回転させる
			if (dot <= -1.0f + 1e-6f) {
				Vector3 axis = Math::Cross(up, kForward);
				// upも平行な場合はX軸を代替軸にする
				if (Math::Length(axis) < 1e-6f) {
					axis = Math::Cross({ 1.0f, 0.0f, 0.0f }, kForward);
				}
				axis = Math::Normalize(axis);
				return Math::MakeRotateAxisAngleQuaternion(axis, static_cast<float>(M_PI));
			}

			// kForwardからdirへの回転軸と角度を求める
			Vector3 axis = Math::Normalize(Math::Cross(kForward, dir));
			float angle = std::acos(dot);
			return Math::MakeRotateAxisAngleQuaternion(axis, angle);
		}

		Vector3 DirectionToEuler(const Vector3& direction) {
			// 方向ベクトルを正規化
			float len = Math::Length(direction);
			// 0ベクトル確認
			if (len < 1e-6f) {
				return { 0.0f, 0.0f, 0.0f };
			}
			Vector3 dir = {
				direction.x / len,
				direction.y / len,
				direction.z / len
			};

			// Pitch
			float clampedY = std::clamp(dir.y, -1.0f, 1.0f);
			float pitch = -std::asinf(clampedY);

			// Yaw
			float yaw = std::atan2(dir.x, dir.z);

			// Roll
			float roll = 0.0f;

			return { pitch, yaw, roll };
		}

		float Length(const Vector3& v) {
			return std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
		}

		float Length(const Vector2& v) {
			return std::sqrtf(v.x * v.x + v.y * v.y);
		}

		Vector3 Normalize(const Vector3& v) {
			float length = Math::Length(v);
			if (length == 0.0f) {
				return Vector3(0.0f, 0.0f, 0.0f);
			} else {
				return Vector3(v.x / length, v.y / length, v.z / length);
			}
		}

		Vector2 Normalize(const Vector2& v) {
			float length = Math::Length(v);
			if (length == 0.0f) {
				return Vector2(0.0f, 0.0f);
			} else {
				return Vector2(v.x / length, v.y / length);
			}
		}

		float Dot(const Vector3& v1, const Vector3& v2) {
			return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
		}

		Vector3 Cross(const Vector3& v1, const Vector3& v2) {
			return Vector3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
		}

		Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m) {
			Vector3 result{
				v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
				v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
				v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2]
			};

			return result;
		}

		Vector3 Project(const Vector3& worldPosition, const Vector2& viewport, const float& viewportWidth, const float& viewportHeight, const Matrix4x4& viewProjection) {

			// ビューポート行列
			Matrix4x4 viewportMatrix = Math::MakeViewportMatrix(viewport.x, viewport.y, viewportWidth, viewportHeight, 0, 1);
			// ビュー行列とプロジェクション行列、ビューポート行列を合成する
			Matrix4x4 viewProjectionViewportMatrix = viewProjection * viewportMatrix;
			// ワールド->スクリーン座標変換(3Dから2Dへ)
			Vector3 screenPos = Math::Transforms(worldPosition, viewProjectionViewportMatrix);
			return  screenPos;
		}

		Vector3 PitchToDirection(float pitch) {
			return { 0.0f, std::sinf(pitch), std::cosf(pitch) };
		}

		Vector3 YawToDirection(float yaw) {
			return { std::sinf(yaw), 0.0f, std::cosf(yaw) };
		}

		Vector3 RollToDirection(float roll) {
			return { std::sinf(roll), std::cosf(roll), 0.0f };
		}

		float LerpShortAngle(float a, float b, float t) {
			float diff = b - a;
			// -2pi-2piに補正する
			diff = std::fmodf(diff, TWO_PI);
			// -pi-piに補正する
			if (diff < -PI) {
				diff += TWO_PI;
			} else if (diff > PI) {
				diff -= TWO_PI;
			}
			return a + diff * t;
		}

		float GetAngleDiff(float a, float b) {
			float diff = b - a;

			// -2pi-2piに補正する
			diff = std::fmodf(diff, TWO_PI);

			// -pi ~ piに補正
			if (diff < -PI) {
				diff += TWO_PI;
			} else if (diff > PI) {
				diff -= TWO_PI;
			}

			// 微小差分判定
			if (std::fabsf(diff) < 1.0e-4f) {
				return 0.0f;
			}

			return diff;
		}

		float WrapAngle(float angle) {
			angle = std::fmod(angle, TWO_PI);
			if (angle < 0.0f) {
				angle += TWO_PI;
			}
			return angle;
		}

		Vector3 Max(Vector3 pos1, Vector3 pos2) {
			return Vector3(std::max(pos1.x, pos2.x), std::max(pos1.y, pos2.y), std::max(pos1.z, pos2.z));
		}

		Vector4 MaxVector4(Vector4 pos1, Vector4 pos2) {
			return Vector4(std::max(pos1.x, pos2.x), std::max(pos1.y, pos2.y), std::max(pos1.z, pos2.z), std::max(pos1.w, pos2.w));
		}

		Vector3 Min(Vector3 pos1, Vector3 pos2) {
			return Vector3(std::min(pos1.x, pos2.x), std::min(pos1.y, pos2.y), std::min(pos1.z, pos2.z));
		}

		Vector4 MinVector4(Vector4 pos1, Vector4 pos2) {
			return Vector4(std::min(pos1.x, pos2.x), std::min(pos1.y, pos2.y), std::min(pos1.z, pos2.z), std::min(pos1.w, pos2.w));
		}

		Matrix4x4 Multiply(const Matrix4x4& matrix1, const Matrix4x4& matrix2) {
			Matrix4x4 result;
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					result.m[i][j] = 0;
					for (int k = 0; k < 4; ++k) {
						result.m[i][j] += matrix1.m[i][k] * matrix2.m[k][j];
					}
				}
			}
			return result;
		}

		Matrix4x4 MakeRotateXMatrix(const float& theta) {
			Matrix4x4 result = {
				1, 0, 0, 0,
				0, std::cosf(theta), std::sinf(theta), 0,
				0, -std::sinf(theta), std::cosf(theta), 0,
				0, 0, 0, 1
			};
			return result;
		}

		Matrix4x4 MakeRotateYMatrix(const float& theta) {
			Matrix4x4 result = {
				std::cosf(theta), 0, -std::sinf(theta), 0,
				0, 1, 0, 0,
				std::sinf(theta), 0, std::cosf(theta), 0,
				0, 0, 0, 1
			};
			return result;
		}

		Matrix4x4 MakeRotateZMatrix(const float& theta) {
			Matrix4x4 result = {
				std::cosf(theta), std::sinf(theta), 0, 0,
				-std::sinf(theta), std::cosf(theta), 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			};
			return result;
		}

		Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
			Matrix4x4 result = {
				scale.x, 0, 0, 0,
				0, scale.y, 0, 0,
				0, 0, scale.z, 0,
				0, 0, 0, 1
			};
			return result;
		}

		Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
			Matrix4x4 result = {
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				translate.x, translate.y, translate.z, 1
			};
			return result;
		}

		Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3 translate) {
			Matrix4x4 scaleMatrix = Math::MakeScaleMatrix(scale);
			Matrix4x4 rotateMatrix = Math::Multiply(Math::MakeRotateXMatrix(rotate.x), Math::Multiply(Math::MakeRotateYMatrix(rotate.y), Math::MakeRotateZMatrix(rotate.z)));
			Matrix4x4 transformMatrix = Math::Multiply(scaleMatrix, rotateMatrix);
			Matrix4x4 result = transformMatrix;
			result.m[3][0] = translate.x;
			result.m[3][1] = translate.y;
			result.m[3][2] = translate.z;
			result.m[3][3] = 1;
			return result;
		}

		Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& quaternion, const Vector3 translate) {

			// 回転行列
			Matrix4x4 rotateMatrix = Math::MakeRotateMatrix(quaternion);

			// 拡縮行列
			Matrix4x4 scaleMatrix = {
				scale.x, 0.0f,   0.0f,   0.0f,
				0.0f,   scale.y, 0.0f,   0.0f,
				0.0f,   0.0f,   scale.z, 0.0f,
				0.0f,   0.0f,   0.0f,    1.0f
			};

			// 平行移動行列
			Matrix4x4 translateMatrix = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				translate.x, translate.y, translate.z, 1.0f
			};

			// SRT行列
			Matrix4x4 worldMatrix = Math::Multiply(Math::Multiply(scaleMatrix, rotateMatrix), translateMatrix);
			return worldMatrix;
		}

		Matrix4x4 InverseMatrix(const Matrix4x4& matrix) {
			Matrix4x4 result;
			float det = matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][3] +
				matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][1] +
				matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][2] +
				matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][2] +
				matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][3] +
				matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][0] +
				matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][3] +
				matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][0] +
				matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][1] +
				matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][1] +
				matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][2] +
				matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][0] -
				matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][2] -
				matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][3] -
				matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][1] -
				matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][3] -
				matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][0] -
				matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][2] -
				matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][1] -
				matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][3] -
				matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][0] -
				matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][2] -
				matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][0] -
				matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][1];
			result.m[0][0] = (matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][3] + matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][1] + matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][2] - matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][2] - matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][3] - matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][1]) / det;
			result.m[0][1] = (matrix.m[0][1] * matrix.m[2][3] * matrix.m[3][2] + matrix.m[0][2] * matrix.m[2][1] * matrix.m[3][3] + matrix.m[0][3] * matrix.m[2][2] * matrix.m[3][1] - matrix.m[0][1] * matrix.m[2][2] * matrix.m[3][3] - matrix.m[0][2] * matrix.m[2][3] * matrix.m[3][1] - matrix.m[0][3] * matrix.m[2][1] * matrix.m[3][2]) / det;
			result.m[0][2] = (matrix.m[0][1] * matrix.m[1][2] * matrix.m[3][3] + matrix.m[0][2] * matrix.m[1][3] * matrix.m[3][1] + matrix.m[0][3] * matrix.m[1][1] * matrix.m[3][2] - matrix.m[0][1] * matrix.m[1][3] * matrix.m[3][2] - matrix.m[0][2] * matrix.m[1][1] * matrix.m[3][3] - matrix.m[0][3] * matrix.m[1][2] * matrix.m[3][1]) / det;
			result.m[0][3] = (matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][2] + matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][3] + matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][1] - matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][3] - matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][1] - matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][2]) / det;
			result.m[1][0] = (matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][2] + matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][3] + matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][0] - matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][3] - matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][0] - matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][2]) / det;
			result.m[1][1] = (matrix.m[0][0] * matrix.m[2][2] * matrix.m[3][3] + matrix.m[0][2] * matrix.m[2][3] * matrix.m[3][0] + matrix.m[0][3] * matrix.m[2][0] * matrix.m[3][2] - matrix.m[0][0] * matrix.m[2][3] * matrix.m[3][2] - matrix.m[0][2] * matrix.m[2][0] * matrix.m[3][3] - matrix.m[0][3] * matrix.m[2][2] * matrix.m[3][0]) / det;
			result.m[1][2] = (matrix.m[0][0] * matrix.m[1][3] * matrix.m[3][2] + matrix.m[0][2] * matrix.m[1][0] * matrix.m[3][3] + matrix.m[0][3] * matrix.m[1][2] * matrix.m[3][0] - matrix.m[0][0] * matrix.m[1][2] * matrix.m[3][3] - matrix.m[0][2] * matrix.m[1][3] * matrix.m[3][0] - matrix.m[0][3] * matrix.m[1][0] * matrix.m[3][2]) / det;
			result.m[1][3] = (matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][3] + matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][0] + matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][2] - matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][3] - matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][0]) / det;
			result.m[2][0] = (matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][3] + matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][0] + matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][1] - matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][1] - matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][3] - matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][0]) / det;
			result.m[2][1] = (matrix.m[0][0] * matrix.m[2][3] * matrix.m[3][1] + matrix.m[0][1] * matrix.m[2][0] * matrix.m[3][3] + matrix.m[0][3] * matrix.m[2][1] * matrix.m[3][0] - matrix.m[0][0] * matrix.m[2][1] * matrix.m[3][3] - matrix.m[0][1] * matrix.m[2][3] * matrix.m[3][0] - matrix.m[0][3] * matrix.m[2][0] * matrix.m[3][1]) / det;
			result.m[2][2] = (matrix.m[0][0] * matrix.m[1][1] * matrix.m[3][3] + matrix.m[0][1] * matrix.m[1][3] * matrix.m[3][0] + matrix.m[0][3] * matrix.m[1][0] * matrix.m[3][1] - matrix.m[0][0] * matrix.m[1][3] * matrix.m[3][1] - matrix.m[0][1] * matrix.m[1][0] * matrix.m[3][3] - matrix.m[0][3] * matrix.m[1][1] * matrix.m[3][0]) / det;
			result.m[2][3] = (matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][1] + matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][3] + matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][0] - matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][3] - matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][0] - matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][1]) / det;
			result.m[3][0] = (matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][1] + matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][2] + matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][0] - matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][2] - matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][0] - matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][1]) / det;
			result.m[3][1] = (matrix.m[0][0] * matrix.m[2][1] * matrix.m[3][2] + matrix.m[0][1] * matrix.m[2][2] * matrix.m[3][0] + matrix.m[0][2] * matrix.m[2][0] * matrix.m[3][1] - matrix.m[0][0] * matrix.m[2][2] * matrix.m[3][1] - matrix.m[0][1] * matrix.m[2][0] * matrix.m[3][2] - matrix.m[0][2] * matrix.m[2][1] * matrix.m[3][0]) / det;
			result.m[3][2] = (matrix.m[0][0] * matrix.m[1][2] * matrix.m[3][1] + matrix.m[0][1] * matrix.m[1][0] * matrix.m[3][2] + matrix.m[0][2] * matrix.m[1][1] * matrix.m[3][0] - matrix.m[0][0] * matrix.m[1][1] * matrix.m[3][2] - matrix.m[0][1] * matrix.m[1][2] * matrix.m[3][0] - matrix.m[0][2] * matrix.m[1][0] * matrix.m[3][1]) / det;
			result.m[3][3] = (matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] + matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] + matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1] - matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1] - matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0]) / det;
			return result;
		}

		Matrix4x4 Transpose(const Matrix4x4& matrix) {
			Matrix4x4 result;
			for (int y = 0; y < 4; ++y) {
				for (int x = 0; x < 4; ++x) {
					result.m[y][x] = matrix.m[x][y];
				}
			}
			return result;
		}

		Matrix4x4 InverseTranspose(const Matrix4x4& matrix) {
			Matrix4x4 result = Math::InverseMatrix(matrix);
			return Math::Transpose(result);
		}

		Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
			float h = 1 / std::tanf(fovY / 2);
			float w = h / aspectRatio;
			Matrix4x4 result = {
				w, 0, 0, 0,
				0, h, 0, 0,
				0, 0, farClip / (farClip - nearClip), 1,
				0, 0, -nearClip * farClip / (farClip - nearClip), 0
			};
			return result;
		}

		Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
			Matrix4x4 result = {
				2.0f / (right - left), 0.0f, 0.0f, 0.0f,
				0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f / (nearClip - farClip), 0.0f,
				(left + right) / (left - right), (top + bottom) / (bottom - top), nearClip / (nearClip - farClip), 1.0f
			};
			return result;
		}

		Vector3 Transforms(const Vector3& vector, const Matrix4x4& matrix) {
			Vector3 result;
			result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + matrix.m[3][0];
			result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + matrix.m[3][1];
			result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + matrix.m[3][2];
			// 同次座標に変換するために 4D ベクトルを使う
			float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + matrix.m[3][3];
			assert(w != 0.0f);
			result.x /= w;
			result.y /= w;
			result.z /= w;
			return result;
		}

		Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minD, float maxD) {
			Matrix4x4 result = {
				width / 2, 0, 0, 0,
				0, -height / 2, 0, 0,
				0, 0, maxD - minD, 0,
				left + width / 2, top + height / 2, minD, 1
			};
			return result;
		}

		Matrix4x4 MakeBillboardMatrix(const Vector3& scale, const Vector3& translate, const Matrix4x4& cameraMatrix) {

			// ビルボードの回転行列を作成
			Matrix4x4 backToFrontMatrix = Math::MakeRotateYMatrix(0.0f);
			Matrix4x4 billboardMatrix = Math::Multiply(backToFrontMatrix, cameraMatrix);
			billboardMatrix.m[3][0] = 0.0f;
			billboardMatrix.m[3][1] = 0.0f;
			billboardMatrix.m[3][2] = 0.0f;
			// ST行列を作成
			Matrix4x4 scaleMatrix = Math::MakeScaleMatrix(scale);
			Matrix4x4 translateMatrix = Math::MakeTranslateMatrix(translate);
			// 行列の更新
			return scaleMatrix * billboardMatrix * translateMatrix;
		}

		Matrix4x4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {

			// カメラの方向ベクトル
			Vector3 z = Math::Normalize(center - eye); // 前方向ベクトル
			Vector3 x = Math::Normalize(Math::Cross(up, z)); // 右方向ベクトル
			Vector3 y = Math::Cross(z, x);             // 上方向ベクトル

			float tx = Math::Dot(x, eye);
			float ty = Math::Dot(y, eye);
			float tz = Math::Dot(z, eye);

			Matrix4x4 result = { {
				{ x.x,  x.y,  x.z,  0.0f },
				{ y.x,  y.y,  y.z,  0.0f },
				{ z.x,  z.y,  z.z,  0.0f },
				{ tx,   ty,   tz,   1.0f }
			} };
			return result;
		}
	}
}
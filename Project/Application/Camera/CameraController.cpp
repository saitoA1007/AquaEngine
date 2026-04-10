#include"CameraController.h"
#include"MyMath.h"
using namespace GameEngine;

void CameraController::Initialize() {
	// カメラの初期化
	camera_ = std::make_unique<Camera>();
	camera_->Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},position_ }, 1280, 720);
}

void CameraController::Update(GameEngine::InputCommand* inputCommand, const Vector3& targetPos) {

	// 注目する位置を取得
	Vector3 target = targetPos;
	target.y = 1.0f;

	// カメラ操作
	if (inputCommand->IsCommandAcitve("CameraMoveLeft")) {
		rotateMove_.x += 0.02f;
	}

	if (inputCommand->IsCommandAcitve("CameraMoveRight")) {
		rotateMove_.x -= 0.02f;
	}

	// 球面座標系で移動
	position_.x = target.x + kDistance_ * std::sinf(rotateMove_.y) * std::sinf(rotateMove_.x);
	position_.y = target.y + kDistance_ * std::cosf(rotateMove_.y);
	position_.z = target.z + kDistance_ * std::sinf(rotateMove_.y) * std::cosf(rotateMove_.x);

	// 回転行列に変換
	Matrix4x4 rotateMatrix_ = LookAt(position_, target, { 0.0f,1.0f,0.0f });

	// ワールド行列
	Matrix4x4 worldMatrix_ = rotateMatrix_;
	worldMatrix_.m[3][0] = position_.x;
	worldMatrix_.m[3][1] = position_.y;
	worldMatrix_.m[3][2] = position_.z;

	// ワールド行列を設定
	camera_->SetWorldMatrix(worldMatrix_);
	// ワールド行列から更新する
	camera_->UpdateFromWorldMatrix();
}

Matrix4x4 CameraController::LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
	Vector3 f = Normalize(center - eye); // 前方向ベクトル
	Vector3 s = Normalize(Cross(up, f)); // 右方向ベクトル
	Vector3 u = Cross(f, s); // 上方向ベクトル

	Matrix4x4 result = { {
		{ s.x,  s.y, s.z, 0 },
		{ u.x,  u.y, u.z, 0 },
		{ f.x,  f.y, f.z, 0 },
		{ 0.0f, 0.0f, 0.0f, 1}
	} };
	return result;
}
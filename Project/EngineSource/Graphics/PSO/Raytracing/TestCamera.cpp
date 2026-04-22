#include "TestCamera.h"
#include "MyMath.h"
#include "CreateBufferResource.h"
#include <algorithm>

using namespace GameEngine;

TestCamera::TestCamera(Input* input) {
	input_ = input;
}

TestCamera::~TestCamera() {

}

void TestCamera::Initialize(const Vector3& translate, int width, int height) {
	translate_ = translate;
	viewMatrix_ = InverseMatrix(MakeAffineMatrix(scale_, rotate_, translate_));
	projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 200.0f);
	rotateMatrix_ = LookAt(translate_, targetPos_, { 0.0f,1.0f,0.0f });

	// 定数バッファの作成
	constBuffer_.Create();
	cameraForGPU_ = constBuffer_.GetData();

	// 球面座標系で移動
	translate_.x = targetPos_.x + distance_ * std::sinf(mouseMove_.y) * std::sinf(mouseMove_.x);
	translate_.y = targetPos_.y + distance_ * std::cosf(mouseMove_.y);
	translate_.z = targetPos_.z + distance_ * std::sinf(mouseMove_.y) * std::cosf(mouseMove_.x);
	// 回転行列に変換
	rotateMatrix_ = LookAt(translate_, targetPos_, { 0.0f,1.0f,0.0f });
	// ワールド行列
	worldMatrix_ = rotateMatrix_;
	worldMatrix_.m[3][0] = translate_.x;
	worldMatrix_.m[3][1] = translate_.y;
	worldMatrix_.m[3][2] = translate_.z;

	// カメラの変更した内容を適用する処理
	viewMatrix_ = InverseMatrix(worldMatrix_);

	Vector3 dir = Vector3(-0.5f, -1.0f, -0.5f);
	dir = Normalize(dir);

	cameraForGPU_->lightDirection = Vector4(dir.x, dir.y, dir.z,1.0f); // 平行光源の向き.
	cameraForGPU_->lightColor = Vector4(1.0f, 1.0f, 1.0f, 0.0f);    // 平行光源色.
	cameraForGPU_->ambientColor = Vector4(0.2f, 0.2f, 0.2f, 0.0f);  // 環境光.
}

void TestCamera::Update() {

	// 中クリックで移動
	if (input_->PushMouse(2)) {

		// ターゲットの移動量
		Vector3 targetMove{ 0.0f,0.0f,0.0f };

		// X軸の移動
		if (input_->GetMouseDelta().x > 0.0f) { targetMove.x = -1.0f; }
		if (input_->GetMouseDelta().x < 0.0f) { targetMove.x = 1.0f; }
		// Y軸の移動
		if (input_->GetMouseDelta().y < 0.0f) { targetMove.y = -1.0f; }
		if (input_->GetMouseDelta().y > 0.0f) { targetMove.y = 1.0f; }

		// カメラの向き
		Vector3 forward = Normalize(targetPos_ - translate_);
		// 上方向
		Vector3 up = { 0.0f, 1.0f, 0.0f };
		// 横方向
		Vector3 right = Normalize(Cross(up, forward));
		// カメラから見てx,y軸に移動量を求める
		Vector3 moveVec = right * targetMove.x + up * targetMove.y;
		// ターゲットに加算
		targetPos_ += moveVec * kTargetSpeed;
	} else {

		// ホイールで距離を調整する
		distance_ -= input_->GetWheel() * 0.05f;
		distance_ = std::clamp(distance_, 2.0f, 100.0f);

		// 右クリックで回転する処理
		if (input_->PushMouse(1)) {
			mouseMove_ += input_->GetMouseDelta() * 0.05f;
		}
	}

	// 球面座標系で移動
	translate_.x = targetPos_.x + distance_ * std::sinf(mouseMove_.y) * std::sinf(mouseMove_.x);
	translate_.y = targetPos_.y + distance_ * std::cosf(mouseMove_.y);
	translate_.z = targetPos_.z + distance_ * std::sinf(mouseMove_.y) * std::cosf(mouseMove_.x);

	// 回転行列に変換
	rotateMatrix_ = LookAt(translate_, targetPos_, { 0.0f,1.0f,0.0f });

	// ワールド行列
	worldMatrix_ = rotateMatrix_;
	worldMatrix_.m[3][0] = translate_.x;
	worldMatrix_.m[3][1] = translate_.y;
	worldMatrix_.m[3][2] = translate_.z;

	//cameraForGPU_->worldPosition = GetWorldPosition();
	// カメラの変更した内容を適用する処理
	viewMatrix_ = InverseMatrix(worldMatrix_);
	//cameraForGPU_->vpMatrix = GetVPMatrix();

	cameraForGPU_->mtxView = viewMatrix_;       // ビュー行列.
	cameraForGPU_->mtxProj = projectionMatrix_; // プロジェクション行列.
	cameraForGPU_->mtxViewInv = InverseMatrix(viewMatrix_);    // ビュー逆行列.
	cameraForGPU_->mtxProjInv = InverseMatrix(projectionMatrix_);// プロジェクション逆行列.

	Vector3 dir = Vector3(-0.5f, -1.0f, -0.5f);
	dir = Normalize(dir);

	cameraForGPU_->lightDirection = Vector4(dir.x, dir.y, dir.z, 1.0f); // 平行光源の向き.
	cameraForGPU_->lightColor = Vector4(1.0f, 1.0f, 1.0f, 0.0f);    // 平行光源色.
	cameraForGPU_->ambientColor = Vector4(0.2f, 0.2f, 0.2f, 0.0f);  // 環境光.

	// 初期位置にリセットする
	if (input_->PushKey(DIK_G) && input_->PushKey(DIK_LCONTROL)) {
		targetPos_ = { 0.0f,0.0f,0.0f };
		mouseMove_ = { 3.1f,1.0f };
		distance_ = 40.0f;
	}
}

Matrix4x4 TestCamera::GetVPMatrix() {
	return Multiply(viewMatrix_, projectionMatrix_);
}

Matrix4x4 TestCamera::GetRotateMatrix() {
	return rotateMatrix_;
}

Vector3 TestCamera::GetWorldPosition() {
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得
	worldPos.x = worldMatrix_.m[3][0];
	worldPos.y = worldMatrix_.m[3][1];
	worldPos.z = worldMatrix_.m[3][2];
	return worldPos;
}

Matrix4x4 TestCamera::LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
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
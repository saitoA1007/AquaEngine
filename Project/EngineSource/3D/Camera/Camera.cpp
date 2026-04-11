#include"Camera.h"
#include"EngineSource/Math/MyMath.h"
#include"EngineSource/Common/CreateBufferResource.h"

using namespace GameEngine;

ID3D12Device* Camera::device_ = nullptr;

Camera::~Camera() {
	
}

void Camera::StaticInitialize(ID3D12Device* device) {
	device_ = device;
}

void Camera::Initialize(const Transform& transform, int kClientWidth, int kClientHeight) {
	// Matrixの初期化
	transform_ = transform;
	worldMatrix_ = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	viewMatrix_ = InverseMatrix(worldMatrix_);
	projectionMatrix_ = MakePerspectiveFovMatrix(0.45f, static_cast<float>(kClientWidth) / static_cast<float>(kClientHeight), 0.1f, 200.0f);
	VPMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

	if (device_) {
		// 定数バッファの作成
		constBuffer_.Create(device_);
		cameraForGPU_ = constBuffer_.GetData();
		// 単位行列を書き込んでおく
		cameraForGPU_->worldPosition = GetWorldPosition();
		cameraForGPU_->vpMatrix = MakeIdentity4x4();
	}
}

void Camera::Update() {
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	viewMatrix_ = InverseMatrix(worldMatrix_);
	VPMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

	if (cameraForGPU_) {
		cameraForGPU_->worldPosition = GetWorldPosition();
		cameraForGPU_->vpMatrix = VPMatrix_;
	}	
}

void Camera::UpdateFromWorldMatrix() {
	viewMatrix_ = InverseMatrix(worldMatrix_);
	VPMatrix_ = Multiply(viewMatrix_, projectionMatrix_);

	if (cameraForGPU_) {
		cameraForGPU_->worldPosition = GetWorldPosition();
		cameraForGPU_->vpMatrix = VPMatrix_;
	}
}

Matrix4x4 Camera::MakeWVPMatrix(Matrix4x4 worldMatrix) {
	WVPMatrix_ = Multiply(worldMatrix, Multiply(viewMatrix_, projectionMatrix_));
	return WVPMatrix_;
}

void Camera::SetProjectionMatrix(float fovY, int kClientWidth, int kClientHeight, float nearPlane, float farPlane) {
	this->projectionMatrix_ = MakePerspectiveFovMatrix(fovY, static_cast<float>(kClientWidth) / static_cast<float>(kClientHeight), nearPlane, farPlane);
}

void Camera::SetViewMatrix(const Matrix4x4& viewMatrix) {
	this->viewMatrix_ = viewMatrix;
}

Vector3 Camera::GetWorldPosition() const {
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得
	worldPos.x = worldMatrix_.m[3][0];
	worldPos.y = worldMatrix_.m[3][1];
	worldPos.z = worldMatrix_.m[3][2];
	return worldPos;
}

void Camera::SetCamera(const Camera& camera) {
	transform_ = camera.transform_;
	worldMatrix_ = camera.GetWorldMatrix();
	VPMatrix_ = camera.GetVPMatrix();

	if (cameraForGPU_) {
		cameraForGPU_->worldPosition = camera.GetWorldPosition();
		cameraForGPU_->vpMatrix = VPMatrix_;
	}
}
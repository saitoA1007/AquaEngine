#include"WorldTransform.h"
#include"MyMath.h"
#include"CreateBufferResource.h"
#include"FPSCounter.h"
using namespace GameEngine;

WorldTransform::~WorldTransform() {

}

void WorldTransform::Initialize(const Transform& transform) {
	transform_ = transform;
	worldMatrix_ = MakeWorldMatrixFromEulerRotation(transform_.translate, transform_.rotate, transform_.scale);

	// 定数バッファの作成
	constBuffer_.Create();
	transformationMatrixData_ = constBuffer_.GetData();

	// 単位行列を書き込んでおく
	transformationMatrixData_->World = MakeIdentity4x4();
	transformationMatrixData_->worldInverseTranspose = MakeIdentity4x4();
}

void WorldTransform::UpdateTransformMatrix() {
	worldMatrix_ = MakeWorldMatrixFromEulerRotation(transform_.translate, transform_.rotate, transform_.scale);
	// 親があれば親のワールド行列を掛ける
	if (parent_) {
		worldMatrix_ *= parent_->GetWorldMatrix();
	}
	transformationMatrixData_->World = worldMatrix_;
	transformationMatrixData_->worldInverseTranspose = InverseTranspose(worldMatrix_);
}

void WorldTransform::SetWVPMatrix(const Matrix4x4& localMatrix) {
	transformationMatrixData_->World = Multiply(localMatrix,worldMatrix_);
	transformationMatrixData_->worldInverseTranspose = InverseTranspose(worldMatrix_);
}

Vector3 WorldTransform::GetWorldPosition() const {
	return Vector3(worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]);
}
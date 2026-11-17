#include "pch.h"
#include"WorldTransform.h"
#include"MyMath.h"
#include"FPSCounter.h"
using namespace GameEngine;

WorldTransform::WorldTransform(const Transform& transform) {
	transform_ = transform;
	worldMatrix_ = Math::MakeWorldMatrixFromEulerRotation(transform_.translate, transform_.rotate, transform_.scale);

	// 単位行列を書き込んでおく
	gpuData_.World = Matrix4x4::MakeIdentity();
	gpuData_.worldInverseTranspose = Matrix4x4::MakeIdentity();
}

WorldTransform::~WorldTransform() {

}

void WorldTransform::Initialize(const Transform& transform) {
	transform_ = transform;
	worldMatrix_ = Math::MakeWorldMatrixFromEulerRotation(transform_.translate, transform_.rotate, transform_.scale);

	// 更新
	UpdateTransformMatrix();
}

void WorldTransform::UpdateTransformMatrix() {
	worldMatrix_ = Math::MakeWorldMatrixFromEulerRotation(transform_.translate, transform_.rotate, transform_.scale);
	// 親があれば親のワールド行列を掛ける
	if (parent_) {
		worldMatrix_ *= parent_->GetWorldMatrix();
	}
	gpuData_.World = worldMatrix_;
	gpuData_.worldInverseTranspose = Matrix4x4::InverseTranspose(worldMatrix_);
}

void WorldTransform::UpdateWorldMatrix(const Matrix4x4 worldMatrix) {
	worldMatrix_ = worldMatrix;
	// 親があれば親のワールド行列を掛ける
	if (parent_) {
		worldMatrix_ *= parent_->GetWorldMatrix();
	}
	gpuData_.World = worldMatrix_;
	gpuData_.worldInverseTranspose = Matrix4x4::InverseTranspose(worldMatrix_);
}

void WorldTransform::SetWVPMatrix(const Matrix4x4& localMatrix) {
	gpuData_.World = localMatrix * worldMatrix_;
	gpuData_.worldInverseTranspose = Matrix4x4::InverseTranspose(worldMatrix_);
}

Vector3 WorldTransform::GetWorldPosition() const {
	return Vector3(worldMatrix_.m[3][0], worldMatrix_.m[3][1], worldMatrix_.m[3][2]);
}
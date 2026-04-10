#include"WorldTransform.h"
#include"MyMath.h"
#include"CreateBufferResource.h"
#include"FPSCounter.h"
using namespace GameEngine;

ID3D12Device* WorldTransform::device_ = nullptr;

WorldTransform::~WorldTransform() {
	// マッピングを解除する
	if (transformationMatrixData_) {
		resource_->Unmap(0, nullptr);
		transformationMatrixData_ = nullptr;
	}
}

void WorldTransform::StaticInitialize(ID3D12Device* device) {
	device_ = device;
}

void WorldTransform::Initialize(const Transform& transform) {
	transform_ = transform;
	worldMatrix_ = MakeWorldMatrixFromEulerRotation(transform_.translate, transform_.rotate, transform_.scale);

	// トランスフォーメーション行列リソースを作成
	// TransformationMatrix用のリソースを作る。TransformationMatrix 1つ分のサイズを用意する
	resource_ = CreateBufferResource(device_, sizeof(TransformationMatrix));
	// データを書き込む
	// 書き込むためのアドレスを取得
	resource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));
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
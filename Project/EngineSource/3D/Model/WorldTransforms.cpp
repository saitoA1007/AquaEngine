#include"WorldTransforms.h"
#include"MyMath.h"
#include"CreateBufferResource.h"
#include"DescriptorHandle.h"
using namespace GameEngine;

ID3D12Device* WorldTransforms::device_ = nullptr;
SrvManager* WorldTransforms::srvManager_ = nullptr;

WorldTransforms::~WorldTransforms() {
	
	// srvIndexを解放
	srvManager_->ReleseIndex(srvIndex_);

	if (resource_) {
		if (instancingData_) {
			resource_->Unmap(0, nullptr);
			instancingData_ = nullptr;
		}
		resource_.Reset();
	}

	transformDatas_.clear();
}

void WorldTransforms::StaticInitialize(ID3D12Device* device, SrvManager* srvManager) {
	device_ = device;
	srvManager_ = srvManager;
}

void WorldTransforms::Initialize(const uint32_t& kNumInstance, const Transform& transform) {

	TransformData transformData;
	for (uint32_t i = 0; i < kNumInstance; ++i) {
		transformData.transform = transform;
		transformData.worldMatrix = MakeAffineMatrix(transformData.transform.scale, transformData.transform.rotate, transformData.transform.translate);
		transformData.color = { 1.0f,1.0f,1.0f,1.0f };
		transformData.textureHandle = 0;
		transformDatas_.push_back(transformData);
	}

	// 頂点数を設定
	numInstance_ = kNumInstance;

	// Instancing用のTransformationMatrixリソースを作る
	resource_ = CreateBufferResource(device_, sizeof(ParticleForGPU) * numInstance_);
	// 書き込むためのアドレスを取得
	resource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));
	// 単位行列を書き込んでおく
	for (uint32_t index = 0; index < numInstance_; ++index) {
		instancingData_[index].World = MakeIdentity4x4();
		instancingData_[index].color = { 1.0f,1.0f,1.0f,1.0f };
		instancingData_[index].textureHandle = 0;
	}

	// SRVのインデックスを取得
	srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);
	// SRVの作成
	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = numInstance_;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);
	instancingSrvHandleCPU_ = srvManager_->GetCPUHandle(srvIndex_);
	instancingSrvHandleGPU_ = srvManager_->GetGPUHandle(srvIndex_);
	device_->CreateShaderResourceView(resource_.Get(), &instancingSrvDesc, instancingSrvHandleCPU_);
}

void WorldTransforms::UpdateTransformMatrix(const uint32_t& numInstance) {
	// 数によって更新を変える
	for (uint32_t i = 0; i < numInstance; ++i) {
		transformDatas_[i].worldMatrix = MakeAffineMatrix(transformDatas_[i].transform.scale, transformDatas_[i].transform.rotate, transformDatas_[i].transform.translate);
	}
}

void WorldTransforms::SetWVPMatrix(const uint32_t& numInstance) {
	// 数によって更新を変える
	for (uint32_t i = 0; i < numInstance; ++i) {
		instancingData_[i].World = transformDatas_[i].worldMatrix;
		instancingData_[i].color = transformDatas_[i].color;
		instancingData_[i].textureHandle = transformDatas_[i].textureHandle;
	}
}

void WorldTransforms::SetWVPMatrix(const uint32_t& numInstance, const Matrix4x4& localMatrix) {

	// 数によって更新を変える
	for (uint32_t i = 0; i < numInstance; ++i) {
		instancingData_[i].World = Multiply(localMatrix, transformDatas_[i].worldMatrix);
		instancingData_[i].color = transformDatas_[i].color;
		instancingData_[i].textureHandle = transformDatas_[i].textureHandle;
	}
}
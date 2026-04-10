#include"Particle.h"

#include"CreateBufferResource.h"
#include"DescriptorHandle.h"

#include"EngineSource/Math/MyMath.h"

using namespace GameEngine;

ID3D12Device* Particles::device_ = nullptr;
ID3D12GraphicsCommandList* Particles::commandList_ = nullptr;
ParticleCSPSO* Particles::particleCSPSO_ = nullptr;
TextureManager* Particles::textureManager_ = nullptr;
uint32_t Particles::StaticSrvIndex_ = 0;

void Particles::Initialize() {
	// メッシュを作成
	mesh_ = std::make_unique<Mesh>();
	mesh_->CreatePlaneMesh(device_, { 1.0f,1.0f });

	const uint32_t kMaxParticles = 1024;

	// リソース作成
	computeResource_ = CreateBufferResource(device_, sizeof(ParticleCS) * kMaxParticles);

	// パーティクルのデータを初期化
	computeResource_->Map(0, nullptr, reinterpret_cast<void**>(&particleData_));
	for (uint32_t i = 0; i < kMaxParticles; ++i) {
		particleData_[i].translate = { 0.0f,0.0f,0.0f };
		particleData_[i].scale = { 1.0f,1.0f,1.0f };
		particleData_[i].lifeTime = 3.0f;
		particleData_[i].velocity = { 0.0f,0.0f,0.0f };
		particleData_[i].currentTime = 2.0f;
		particleData_[i].color = { 1.0f,1.0f,1.0f,1.0f };
	}

	// UAVを作成
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = kMaxParticles;
	uavDesc.Buffer.StructureByteStride = sizeof(ParticleCS);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	//uavHandleCPU_ = GetCPUDescriptorHandle(dxCommon_->GetSRVHeap(), dxCommon_->GetSRVDescriptorSize(), StaticSrvIndex_ + static_cast<uint32_t>(ResourceCount::kStartModelCount));
	//uavHandleGPU_ = GetGPUDescriptorHandle(dxCommon_->GetSRVHeap(), dxCommon_->GetSRVDescriptorSize(), StaticSrvIndex_ + static_cast<uint32_t>(ResourceCount::kStartModelCount));
	//dxCommon_->GetDevice()->CreateUnorderedAccessView(computeResource_.Get(), nullptr, &uavDesc, uavHandleCPU_);

	// srvを更新
	StaticSrvIndex_++;

	// TransformationMatrix用のリソースを作る。TransformationMatrix 1つ分のサイズを用意する
	cameraResource_ = CreateBufferResource(device_, sizeof(TransformationMatrix));
	// データを書き込む
	// 書き込むためのアドレスを取得
	cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&PerViewData_));
	// 単位行列を書き込んでおく
	PerViewData_->viewProjection = MakeIdentity4x4();
	PerViewData_->billboardMatrix = MakeIdentity4x4();
}

void Particles::Update() {

	// コンピュートシェーダーでパーティクルを更新
	commandList_->SetComputeRootSignature(particleCSPSO_->GetComputeRootSignature());
	commandList_->SetPipelineState(particleCSPSO_->GetComputePipelineState());

	// UAVをセット
	commandList_->SetComputeRootDescriptorTable(0, uavHandleGPU_);
	
	// UAV,CBVなどをセット
	commandList_->Dispatch(1,1,1);

	// UAVバリアを貼る
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = computeResource_.Get();
	commandList_->ResourceBarrier(1, &barrier);
}

void Particles::Draw(const Matrix4x4& VPMatrix, const uint32_t& textureHandle) {

	// カメラ位置を更新
	PerViewData_->viewProjection = VPMatrix;

	commandList_->SetGraphicsRootSignature(particleCSPSO_->GetRootSignature());
	commandList_->SetPipelineState(particleCSPSO_->GetPipelineState());

	commandList_->IASetVertexBuffers(0, 1, &mesh_->GetVertexBufferView());
	commandList_->IASetIndexBuffer(&mesh_->GetIndexBufferView());
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// カメラ位置
	commandList_->SetGraphicsRootConstantBufferView(0, cameraResource_->GetGPUVirtualAddress());
	// パーティクルデータ
	commandList_->SetGraphicsRootDescriptorTable(1, uavHandleGPU_);
	// テクスチャ
	commandList_->SetGraphicsRootDescriptorTable(2, textureManager_->GetTextureSrvHandlesGPU(textureHandle));

	// 描画
	commandList_->DrawIndexedInstanced(mesh_->GetTotalIndices(), 1024, 0, 0, 0);
}
#include "ParticleBehaviorGPU.h"
#include "PSOManager.h"
using namespace GameEngine;

ID3D12GraphicsCommandList4* ParticleBehaviorGPU::commandList_ = nullptr;
ID3D12RootSignature* ParticleBehaviorGPU::rootSignature_ = nullptr;
ID3D12PipelineState* ParticleBehaviorGPU::pipelineState_ = nullptr;

void ParticleBehaviorGPU::StaticInitialize(ID3D12GraphicsCommandList4* commandList, PSOManager* psoManager) {
	commandList_ = commandList;

	auto psoData = psoManager->GetDrawPsoData("ComputeAnimation");
	pipelineState_ = psoData.graphicsPipelineState;
	rootSignature_ = psoData.rootSignature;
}

ParticleBehaviorGPU::ParticleBehaviorGPU() {

}

void ParticleBehaviorGPU::Initialize() {

}

void ParticleBehaviorGPU::Update() {

	commandList_->SetComputeRootSignature(rootSignature_);
	commandList_->SetPipelineState(pipelineState_);

	//auto* skeleton = model_->GetSkeleton();
	//const auto& meshes = model_->GetMeshes();
	//
	//auto* outputBuffer = skeleton->GetOutputVertexBuffer(i);
	//outputBuffer->TransitionUAV(commandList_);
	//
	//commandList_->SetComputeRootDescriptorTable(0, skinCluster_->wellBuffer.GetSrvHandleGPU()); // 共通パレット
	//commandList_->SetComputeRootDescriptorTable(1, meshes[i]->GetVertexBuffer().GetSrvGpuHandle());
	//commandList_->SetComputeRootDescriptorTable(2, skeleton->GetInfluenceBuffer(i)->GetSrvGpuHandle()); // メッシュ固有
	//commandList_->SetComputeRootDescriptorTable(3, outputBuffer->GetUAVGpuHandle());
	//commandList_->SetComputeRootConstantBufferView(4, skeleton->GetConstantBuffer(i)->GetGpuVirtualAddress());
	//
	//commandList_->Dispatch(UINT(skeleton->GetVerticesNum(i) + 1023) / 1024, 1, 1);
	//
	//outputBuffer->TransitionSRV(commandList_);
}

void ParticleBehaviorGPU::Draw() {

}
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

ParticleBehaviorGPU::ParticleBehaviorGPU(const std::string& name, uint32_t maxNum) {

	name_ = name;

	// パーティクルの数
	std::vector<ParticleCS> particleData;
	particleData.resize(maxNum);

	particleBuffer_.Create(commandList_, particleData);

}

void ParticleBehaviorGPU::Initialize() {

}

void ParticleBehaviorGPU::Update() {

	commandList_->SetComputeRootSignature(rootSignature_);
	commandList_->SetPipelineState(pipelineState_);

	particleBuffer_.TransitionUAV(commandList_);

	// パーティクルを更新
	commandList_->SetComputeRootDescriptorTable(0, particleBuffer_.GetUAVGpuHandle());	
	commandList_->Dispatch(1024, 1, 1);
	
	particleBuffer_.TransitionSRV(commandList_);
}

void ParticleBehaviorGPU::Draw() {

}
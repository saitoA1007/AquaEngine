#include "pch.h"
#include "RayQueryPipeline.h"
#include <cassert>
#include "LogManager.h"
#include "PSOManager.h"
using namespace GameEngine;

namespace {
	constexpr uint32_t kMaterialCount = static_cast<uint32_t>(RayQueryMaterial::kCount);
}

void RayQueryPipeline::Initialize(ID3D12Device5* device, PSOManager* psoManager, uint32_t width, uint32_t height) {
	device_ = device;
	width_ = width;
	height_ = height;

	// PSOManagerに登録済みのルートシグネチャ、PSO、コマンドシグネチャを受け取る
	rootSignature_ = psoManager->GetRootSignature(RayQueryPsoName::kRootSignature);
	resetPso_ = psoManager->GetPSO(RayQueryPsoName::kReset);
	tracePso_ = psoManager->GetPSO(RayQueryPsoName::kTrace);
	buildArgsPso_ = psoManager->GetPSO(RayQueryPsoName::kBuildArgs);
	aoTracePso_ = psoManager->GetPSO(RayQueryPsoName::kAOTrace);
	aoTemporalPso_ = psoManager->GetPSO(RayQueryPsoName::kAOTemporal);
	aoBlurPso_ = psoManager->GetPSO(RayQueryPsoName::kAOBlur);
	for (uint32_t i = 0; i < kMaterialCount; ++i) {
		shadePsos_[i] = psoManager->GetPSO(RayQueryPsoName::kShade[i]);
	}
	dispatchSignature_ = psoManager->GetCommandSignature(RayQueryPsoName::kDispatchSignature);

	// 中間バッファ
	CreateBuffers();

	LogManager::GetInstance().Log("RayQueryPipeline initialized");
}

void RayQueryPipeline::CreateBuffers() {
	const uint32_t pixelCount = width_ * height_;
	hitBuffer_.CreateForUAV(pixelCount);
	// マテリアルごとに全ピクセル分の領域を持つ
	pixelList_.CreateForUAV(pixelCount * kMaterialCount);
	counter_.CreateForUAV(kMaterialCount);
	dispatchArgs_.CreateForUAV(kMaterialCount);
	// RTAO
	aoHistory_[0].CreateForUAV(pixelCount);
	aoHistory_[1].CreateForUAV(pixelCount);
	aoOutput_.CreateForUAV(pixelCount);
	prevCamera_.CreateForUAV(1);
}

void RayQueryPipeline::UavBarrier(ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* resource) {
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = resource;
	cmdList->ResourceBarrier(1, &barrier);
}

void RayQueryPipeline::Execute(ID3D12GraphicsCommandList4* cmdList, const RayQueryFrameParams& params) {
	// 全パス共通のルート引数
	RayQueryIndices indices{};
	indices.tlas = params.tlasSrvIndex;
	indices.bufferRefs = params.bufferRefSrvIndex;
	indices.output = params.outputUavIndex;
	indices.depthOutput = params.depthOutputUavIndex;
	indices.skybox = params.skyboxSrvIndex;
	indices.hitBuffer = hitBuffer_.GetUAVIndex();
	indices.pixelList = pixelList_.GetUAVIndex();
	indices.counter = counter_.GetUAVIndex();
	indices.dispatchArgs = dispatchArgs_.GetUAVIndex();
	indices.width = width_;
	indices.height = height_;
	// RTAO
	const uint32_t current = frameIndex_ % 2;
	indices.frameIndex = frameIndex_;
	indices.aoHistoryCurrent = aoHistory_[current].GetUAVIndex();
	indices.aoHistoryPrevious = aoHistory_[1 - current].GetUAVIndex();
	indices.aoOutput = aoOutput_.GetUAVIndex();
	indices.prevCamera = prevCamera_.GetUAVIndex();
	indices.aoEnabled = rtaoSettings_.enabled ? 1 : 0;
	indices.aoSampleCount = rtaoSettings_.sampleCount;
	indices.aoRadius = rtaoSettings_.radius;
	indices.aoStrength = rtaoSettings_.strength;
	indices.ambientIntensity = rtaoSettings_.ambientIntensity;
	indices.aoDebugView = (rtaoSettings_.enabled && rtaoSettings_.debugView) ? 1 : 0;
	indices.aoHistoryClamp = rtaoSettings_.historyClamp ? 1 : 0;
	indices.aoClampGamma = rtaoSettings_.clampGamma;
	indices.texLodEnabled = textureLodSettings_.enabled ? 1 : 0;
	indices.texLodBias = textureLodSettings_.bias;
	++frameIndex_;

	cmdList->SetComputeRootSignature(rootSignature_);
	cmdList->SetComputeRoot32BitConstants(static_cast<UINT>(RayQueryRootParam::kIndices),
		sizeof(indices) / sizeof(uint32_t), &indices, 0);
	cmdList->SetComputeRootConstantBufferView(static_cast<UINT>(RayQueryRootParam::kCamera), params.cameraAddress);
	cmdList->SetComputeRootConstantBufferView(static_cast<UINT>(RayQueryRootParam::kLight), params.lightAddress);
	cmdList->SetComputeRootShaderResourceView(static_cast<UINT>(RayQueryRootParam::kInstanceDescs), params.instanceDescAddress);

	// ピクセル数をリセット
	cmdList->SetPipelineState(resetPso_);
	cmdList->Dispatch(1, 1, 1);
	UavBarrier(cmdList, counter_.GetResource());

	// 1次レイを飛ばしてマテリアルごとに振り分ける
	cmdList->SetPipelineState(tracePso_);
	cmdList->Dispatch((width_ + kTraceGroupSize - 1) / kTraceGroupSize, (height_ + kTraceGroupSize - 1) / kTraceGroupSize, 1);
	// ヒット情報、ピクセルリスト、カウンタ、出力をまとめて待つ
	UavBarrier(cmdList, nullptr);

	// RTAO
	if (rtaoSettings_.enabled) {
		const UINT groupX = (width_ + kTraceGroupSize - 1) / kTraceGroupSize;
		const UINT groupY = (height_ + kTraceGroupSize - 1) / kTraceGroupSize;

		cmdList->SetPipelineState(aoTracePso_);
		cmdList->Dispatch(groupX, groupY, 1);
		UavBarrier(cmdList, aoHistory_[current].GetResource());

		cmdList->SetPipelineState(aoTemporalPso_);
		cmdList->Dispatch(groupX, groupY, 1);
		UavBarrier(cmdList, aoHistory_[current].GetResource());

		cmdList->SetPipelineState(aoBlurPso_);
		cmdList->Dispatch(groupX, groupY, 1);
		UavBarrier(cmdList, aoOutput_.GetResource());
	}

	// Dispatch引数を作る
	cmdList->SetPipelineState(buildArgsPso_);
	cmdList->Dispatch(1, 1, 1);
	UavBarrier(cmdList, nullptr); // Dispatch引数と前フレームのカメラ
	TransitionResource(cmdList, dispatchArgs_.GetResource(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	// マテリアルごとに専用のPSOでシェーディングする
	for (uint32_t i = 0; i < kMaterialCount; ++i) {
		cmdList->SetPipelineState(shadePsos_[i]);
		cmdList->ExecuteIndirect(dispatchSignature_, 1,
			dispatchArgs_.GetResource(), sizeof(D3D12_DISPATCH_ARGUMENTS) * i, nullptr, 0);
	}

	// 次のフレームで書き込めるように戻す
	TransitionResource(cmdList, dispatchArgs_.GetResource(),
		D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

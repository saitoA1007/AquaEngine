#include "pch.h"
#include "RayQueryPipeline.h"
#include <algorithm>
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
	causticClearPso_ = psoManager->GetPSO(RayQueryPsoName::kCausticClear);
	causticBoundsPso_ = psoManager->GetPSO(RayQueryPsoName::kCausticBounds);
	causticTracePso_ = psoManager->GetPSO(RayQueryPsoName::kCausticTrace);
	causticResolvePso_ = psoManager->GetPSO(RayQueryPsoName::kCausticResolve);
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
	// コースティクス
	causticBounds_.CreateForUAV(4);
	causticAccum_.CreateForUAV(pixelCount * 3);
	causticOutput_.CreateForUAV(pixelCount);
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
	// コースティクス
	const bool causticEnabled = causticSettings_.enabled;
	indices.causticEnabled = causticEnabled ? 1 : 0;
	indices.causticBounds = causticBounds_.GetUAVIndex();
	indices.causticAccum = causticAccum_.GetUAVIndex();
	indices.causticOutput = causticOutput_.GetUAVIndex();
	indices.causticCoarseResolution = kCausticCoarseResolution;
	indices.causticPhotonResolution = std::clamp(causticSettings_.photonResolution, 1u, kMaxCausticPhotonResolution);
	indices.causticEmitRange = (std::max)(causticSettings_.emitRange, 0.1f);
	indices.causticGatherRadius = std::clamp(causticSettings_.gatherRadius, 1u, kMaxCausticGatherRadius);
	indices.causticIntensity = causticSettings_.intensity;
	indices.causticIceShadowDensity = std::clamp(causticSettings_.iceShadowDensity, 0.0f, 1.0f);
	indices.causticDebugView = (causticEnabled && causticSettings_.debugView) ? 1 : 0;
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

	// 氷で屈折した光をフォトンで運び、コースティクスの照度を求める
	if (causticEnabled) {
		const UINT pixelGroupX = (width_ + kTraceGroupSize - 1) / kTraceGroupSize;
		const UINT pixelGroupY = (height_ + kTraceGroupSize - 1) / kTraceGroupSize;
		const UINT coarseGroup = (kCausticCoarseResolution + kTraceGroupSize - 1) / kTraceGroupSize;
		const UINT photonGroup = (indices.causticPhotonResolution + kTraceGroupSize - 1) / kTraceGroupSize;

		// 加算先と範囲をリセット
		cmdList->SetPipelineState(causticClearPso_);
		cmdList->Dispatch(pixelGroupX, pixelGroupY, 1);
		UavBarrier(cmdList, nullptr);

		// 粗いフォトンで氷のある範囲を求める
		cmdList->SetPipelineState(causticBoundsPso_);
		cmdList->Dispatch(coarseGroup, coarseGroup, 1);
		UavBarrier(cmdList, causticBounds_.GetResource());

		// 氷の範囲へフォトンを撃ち、着地点のピクセルへ加算する
		cmdList->SetPipelineState(causticTracePso_);
		cmdList->Dispatch(photonGroup, photonGroup, 1);
		UavBarrier(cmdList, causticAccum_.GetResource());

		// 周囲のフォトンを集めて照度にする
		cmdList->SetPipelineState(causticResolvePso_);
		cmdList->Dispatch(pixelGroupX, pixelGroupY, 1);
		UavBarrier(cmdList, causticOutput_.GetResource());
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

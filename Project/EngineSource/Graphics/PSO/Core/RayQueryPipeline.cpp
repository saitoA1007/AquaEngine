#include "RayQueryPipeline.h"
#include <cassert>
#include "EngineSource/Graphics/PSO/Core/RootSignatureBuilder.h"
#include "SrvManager.h"
#include "RayQueryMaterialRegistry.h"
using namespace GameEngine;

void RayQueryPipeline::Initialize(ID3D12Device* device, DXC* dxc) {
	device_ = device;
	dxc_ = dxc;

	// 登録済みのマテリアルを読み込む
	RayQueryMaterialRegistry::GetInstance().Load();

	// ルートシグネチャを作成する
	CreateRootSignature();

	// パイプラインを作成する
	CreatePipelineState();
}

void RayQueryPipeline::ReloadIfNeeded() {
	uint32_t version = RayQueryMaterialRegistry::GetInstance().GetVersion();
	if (version == materialVersion_) { return; }

	// 使用中の可能性があるパイプラインを退避してから作り直す
	retiredPipelineState_ = pipelineState_;
	CreatePipelineState();
}

void RayQueryPipeline::CreateRootSignature() {

	// tlasの設定、カメラ、ライトの設定、マテリアルアクセスデータ、バッファデータの設定
	uint32_t texMaxNum = static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount); // テクスチャ
	uint32_t bufferMaxNum = static_cast<uint32_t>(SrvHeapTypeCount::BufferMaxCount); // データ

	// RootParamの順番と合わせる
	RootSignatureBuilder builder;
	builder.Initialize(device_);
	builder.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_ALL); // tlas
	builder.AddSRVDescriptorTable(0, texMaxNum, 1, D3D12_SHADER_VISIBILITY_ALL); // テクスチャ
	builder.AddSRVDescriptorTable(0, 1, 2, D3D12_SHADER_VISIBILITY_ALL); // アクセスデータ
	builder.AddSRVDescriptorTable(0, bufferMaxNum, 3, D3D12_SHADER_VISIBILITY_ALL); // マテリアルなどのデータ
	builder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL); // camera
	builder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL); // light
	builder.AddUAVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_ALL); // UAV gOutput
	builder.AddUAVDescriptorTable(1, 1, 0, D3D12_SHADER_VISIBILITY_ALL); // UAV gOutputDepth
	builder.AddSRVDescriptorTable(1, 1, 0, D3D12_SHADER_VISIBILITY_ALL); // skybox
	builder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_ALL);
	builder.CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAG_NONE);
	rootSignature_ = builder.MoveOwnerRootSignature();
}

void RayQueryPipeline::CreatePipelineState() {
	// インクルードファイルの変更を確実に反映させるため、キャッシュを使わずにコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> csBlob = dxc_->CompileShader(kShaderPath_, kShaderProfile_.c_str(), L"main");
	assert(csBlob != nullptr && "RayQuery shader compile failed");

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSignature_.Get();
	desc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };

	HRESULT hr = device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr) && "Failed to create RayQuery pipeline state");

	materialVersion_ = RayQueryMaterialRegistry::GetInstance().GetVersion();
}

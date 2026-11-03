#include "pch.h"
#include "RaytracingPipeline.h"
#include "EngineSource/Graphics/PSO/Core/RootSignatureBuilder.h"
using namespace GameEngine;

void RaytracingPipeline::Initialize(ID3D12Device5* device, SrvManager* srvManager, DXC* dxc) {
	device_ = device;
	srvManager_ = srvManager;
	dxc_ = dxc;

	// シェーダコンパイル機能を初期化
	rayLibShaderCompiler_.Initialize(dxc);

	// ルートシグネチャを作成する
	CreateGlobalRootsignature();

	// ステートオブジェクトを作成する
	CreateStateObject();

	// シェーダーテーブルを作成
	CreateShaderTable();

	// RayQuery用のPSOを作成
	CreateRayQueryPipeline();

	// パストレーシングのデノイズ用のPSOを作成
	CreateDenoisePipeline();

	// ビジビリティバッファ用のPSOを作成
	CreateVisibilityPipeline();
}

void RaytracingPipeline::CreateVisibilityPipeline() {
	uint32_t bufferMaxNum = static_cast<uint32_t>(SrvHeapTypeCount::BufferMaxCount);

	// インスタンスの定数、カメラ、アクセスデータ、バッファデータ
	RootSignatureBuilder builder;
	builder.Initialize(device_);
	builder.Add32BitConstantsParameter(kVisibilityConstantCount, 0, D3D12_SHADER_VISIBILITY_ALL);
	builder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_VERTEX); // camera
	builder.AddSRVDescriptorTable(0, 1, 2, D3D12_SHADER_VISIBILITY_VERTEX); // アクセスデータ
	builder.AddSRVDescriptorTable(0, bufferMaxNum, 3, D3D12_SHADER_VISIBILITY_VERTEX); // 頂点・インデックスなどのデータ
	builder.CreateRootSignature();
	rootSignatureVisibility_ = builder.MoveOwnerRootSignature();

	const std::wstring path = L"Resources/Shaders/Raytracing/VisibilityBuffer.hlsl";
	Microsoft::WRL::ComPtr<IDxcBlob> vsBlob = dxc_->CompileShader(path, L"vs_6_6", L"MainVisibilityVS");
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob = dxc_->CompileShader(path, L"ps_6_6", L"MainVisibilityPS");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSignatureVisibility_.Get();
	desc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	desc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;
	// レイトレーシングと同じく裏面も描画する
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R32G32_UINT;
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;

	HRESULT hr = device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&visibilityPipelineState_));
	assert(SUCCEEDED(hr));
}

void RaytracingPipeline::CreateRayQueryPipeline() {
	const std::wstring rayQueryPath = L"Resources/Shaders/Raytracing/RayQuery.CS.hlsl";
	rayQueryPipelineState_ = CreateComputePipeline(rayQueryPath, L"MainRayQueryCS", rootSignatureGlobal_.Get());
	pathTracePipelineState_ = CreateComputePipeline(rayQueryPath, L"MainPathTraceCS", rootSignatureGlobal_.Get());
}

void RaytracingPipeline::CreateDenoisePipeline() {

	// デノイズ用のルートシグネチャ。定数、カメラ、UAV(u0~u11)
	RootSignatureBuilder builder;
	builder.Initialize(device_);
	builder.Add32BitConstantsParameter(4, 0, D3D12_SHADER_VISIBILITY_ALL); // サンプル間隔などの定数
	builder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL); // camera
	for (uint32_t i = 0; i < kDenoiseUavCount; ++i) {
		builder.AddUAVDescriptorTable(i, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
	}
	builder.CreateRootSignature();
	rootSignatureDenoise_ = builder.MoveOwnerRootSignature();

	const std::wstring denoisePath = L"Resources/Shaders/Raytracing/PathTraceDenoise.CS.hlsl";
	temporalPipelineState_ = CreateComputePipeline(denoisePath, L"MainTemporalCS", rootSignatureDenoise_.Get());
	storeCameraPipelineState_ = CreateComputePipeline(denoisePath, L"MainStoreCameraCS", rootSignatureDenoise_.Get());
	atrousPipelineState_ = CreateComputePipeline(denoisePath, L"MainAtrousCS", rootSignatureDenoise_.Get());
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> RaytracingPipeline::CreateComputePipeline(const std::wstring& hlslPath, const std::wstring& entryPoint, ID3D12RootSignature* rootSignature) {
	Microsoft::WRL::ComPtr<IDxcBlob> csBlob = dxc_->CompileShader(hlslPath, L"cs_6_6", entryPoint);

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
	desc.CS = {
		.pShaderBytecode = csBlob->GetBufferPointer(),
		.BytecodeLength = csBlob->GetBufferSize(),
	};
	desc.pRootSignature = rootSignature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	HRESULT hr = device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(hr));
	return pipelineState;
}

void RaytracingPipeline::CreateGlobalRootsignature() {

	// tlasの設定、カメラ、ライトの設定、マテリアルアクセスデータ、バッファデータの設定
	uint32_t texMaxNum = static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount); // テクスチャ
	uint32_t bufferMaxNum = static_cast<uint32_t>(SrvHeapTypeCount::BufferMaxCount); // データ

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
	builder.Add32BitConstantsParameter(2, 2, D3D12_SHADER_VISIBILITY_ALL); // フレーム番号、ビジビリティバッファを使うか(RayQuery用)
	builder.AddUAVDescriptorTable(2, 1, 0, D3D12_SHADER_VISIBILITY_ALL); // UAV デノイズ用の法線・距離(パストレーシング用)
	builder.AddUAVDescriptorTable(3, 1, 0, D3D12_SHADER_VISIBILITY_ALL); // UAV デノイズ用のアルベド(パストレーシング用)
	builder.AddSRVDescriptorTable(2, 1, 0, D3D12_SHADER_VISIBILITY_ALL); // ビジビリティバッファ(パストレーシング用)
	builder.AddSRVParameter(3, D3D12_SHADER_VISIBILITY_ALL); // TLASのインスタンス情報(パストレーシング用)
	builder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_ALL);
	builder.CreateRootSignature();
	rootSignatureGlobal_ = builder.MoveOwnerRootSignature();
}

void RaytracingPipeline::CreateStateObject() {
	LibraryResult raygenResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/RayGen.hlsl");
	LibraryResult missResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/Miss.hlsl");
	LibraryResult objectResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/chsObject.hlsl");
	LibraryResult iceObjectResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/ChsIceObject.hlsl");

	// 初期化処理
	stateObjectBuilder_.Initialize();

	// シェーダーを設定
	stateObjectBuilder_.AddDXILLibrary(raygenResult.blob.Get(), raygenResult.exportNames);
	stateObjectBuilder_.AddDXILLibrary(missResult.blob.Get(), missResult.exportNames);
	stateObjectBuilder_.AddDXILLibrary(objectResult.blob.Get(), objectResult.exportNames);
	stateObjectBuilder_.AddDXILLibrary(iceObjectResult.blob.Get(), iceObjectResult.exportNames);

	// ヒットグループを設定
	stateObjectBuilder_.AddHitGroup(AppHitGroups::DefaultModel, L"MainObjectCHS");
	stateObjectBuilder_.AddHitGroup(AppHitGroups::IceModel, L"MainIceObjectCHS");

	// シェーダー設定
	const uint32_t MaxPayloadSize = sizeof(float) * 3 + sizeof(uint32_t) + sizeof(float);
	const uint32_t MaxAttributeSize = sizeof(float) * 2;
	stateObjectBuilder_.SetShaderConfig(MaxPayloadSize, MaxAttributeSize);
	stateObjectBuilder_.SetPipelineConfig(4);

	// グローバルルートシグネチャを設定
	stateObjectBuilder_.SetGlobalRootSignature(rootSignatureGlobal_.Get());

	// 生成する
	stateObject_ = stateObjectBuilder_.Build(device_);
}

void RaytracingPipeline::CreateShaderTable() {

	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtsoProps;
	stateObject_.As(&rtsoProps);

	// raygen
	{
		auto id = rtsoProps->GetShaderIdentifier(L"MainRayGen");
		if (id == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}

		ShaderRecord record;
		auto table = record.SetIdentifier(id);
		shaderTableBuilder_.RayGen().AddRecord(std::move(record));
	}

	// miss
	{
		auto id = rtsoProps->GetShaderIdentifier(L"MainMiss");
		if (id == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}
		ShaderRecord record;
		record.SetIdentifier(id);
		shaderTableBuilder_.Miss().AddRecord(std::move(record));

		// シャドウ判定用のMissシェーダー
		auto shadowId = rtsoProps->GetShaderIdentifier(L"ShadowMiss");
		if (shadowId == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}
		ShaderRecord shadowRecord;
		shadowRecord.SetIdentifier(shadowId);
		shaderTableBuilder_.Miss().AddRecord(std::move(shadowRecord));
	}
	
	// hitGroup
	{
		// デフォルト
		auto id = rtsoProps->GetShaderIdentifier(AppHitGroups::DefaultModel.c_str());
		if (id == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}
		ShaderRecord record;
		auto& table = record.SetIdentifier(id);
		shaderTableBuilder_.HitGroup().AddRecord(std::move(record));

		// 氷
		auto iceId = rtsoProps->GetShaderIdentifier(AppHitGroups::IceModel.c_str());
		if (iceId == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}
		ShaderRecord iceRecord;
		auto& iceTable = iceRecord.SetIdentifier(iceId);
		shaderTableBuilder_.HitGroup().AddRecord(std::move(iceRecord));
	}

	// テーブルを設定する
	shaderTableBuilder_.Build(device_);

	// 保存する
	dispatchRayDesc_ = shaderTableBuilder_.CreateDispatchRaysDesc(1280, 720);
}
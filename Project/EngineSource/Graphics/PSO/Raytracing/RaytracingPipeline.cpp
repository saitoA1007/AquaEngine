#include "RaytracingPipeline.h"
#include "ModelManager.h"
#include "EngineSource/Graphics/PSO/Core/RootSignatureBuilder.h"
using namespace GameEngine;

void RaytracingPipeline::Initialize(ID3D12Device5* device, SrvManager* srvManager, DXC* dxc,
	RenderPassController* renderPassController, TLAS* tlas) {
	device_ = device;
	srvManager_ = srvManager;
	renderPassController_ = renderPassController;
	tlas_ = tlas;

	// シェーダコンパイル機能を初期化
	rayLibShaderCompiler_.Initialize(dxc);

	// ルートシグネチャを作成する
	CreateGlobalRootsignature();
	CreateLocalRootsignature();

	// ステートオブジェクトを作成する
	CreateStateObject();
}

void RaytracingPipeline::CreateGlobalRootsignature() {

	// tlasの設定、カメラ、ライトの設定、マテリアルアクセスデータ、バッファデータの設定
	uint32_t texMaxNum = static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount); // テクスチャ
	uint32_t systemMaxNum = static_cast<uint32_t>(SrvHeapTypeCount::SystemMaxCount); // ポストエフェクト
	uint32_t accessMaxNum = static_cast<uint32_t>(SrvHeapTypeCount::AccessMaxCount); // アクセスデータ
	uint32_t bufferMaxNum = static_cast<uint32_t>(SrvHeapTypeCount::BufferMaxCount); // データ

	RootSignatureBuilder builder;
	builder.Initialize(device_);
	builder.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_ALL); // tlas
	builder.AddSRVDescriptorTable(0, texMaxNum, 1, D3D12_SHADER_VISIBILITY_ALL); // テクスチャ
	builder.AddSRVDescriptorTable(0, accessMaxNum, 2, D3D12_SHADER_VISIBILITY_ALL, texMaxNum + systemMaxNum); // アクセスデータ
	builder.AddSRVDescriptorTable(0, bufferMaxNum, 3, D3D12_SHADER_VISIBILITY_ALL, texMaxNum + systemMaxNum + accessMaxNum); // マテリアルなどのデータ
	builder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL); // camera
	builder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL); // light
	builder.CreateRootSignature();
	rootSignatureGlobal_ = builder.MoveOwnerRootSignature();
}

void RaytracingPipeline::CreateLocalRootsignature() {

	// raygen用
	RootSignatureBuilder raygenBuilder;
	raygenBuilder.Initialize(device_);
	raygenBuilder.AddUAVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
	raygenBuilder.CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	rsRayGen_ = raygenBuilder.MoveOwnerRootSignature();

	// Object用
	RootSignatureBuilder objectBuilder;
	objectBuilder.Initialize(device_);
	objectBuilder.AddSRVDescriptorTable(0, 1, 4, D3D12_SHADER_VISIBILITY_ALL);
	objectBuilder.AddSRVDescriptorTable(1, 1, 4, D3D12_SHADER_VISIBILITY_ALL);
	objectBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_ALL);
	objectBuilder.CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	rsModel_ = objectBuilder.MoveOwnerRootSignature();
}

void RaytracingPipeline::CreateStateObject() {
	LibraryResult raygenResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/RayGen.hlsl");
	LibraryResult missResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/Miss.hlsl");
	LibraryResult objectResult = rayLibShaderCompiler_.CompileShader(L"Resources/Shaders/Raytracing/chsObject.hlsl");

	// 初期化処理
	stateObjectBuilder_.Initialize();

	// シェーダーを設定
	stateObjectBuilder_.AddDXILLibrary(raygenResult.blob.Get(), raygenResult.exportNames);
	stateObjectBuilder_.AddDXILLibrary(missResult.blob.Get(), missResult.exportNames);
	stateObjectBuilder_.AddDXILLibrary(objectResult.blob.Get(), objectResult.exportNames);

	// ヒットグループを設定
	stateObjectBuilder_.AddHitGroup(AppHitGroups::DefaultModel, L"MainObjectCHS");

	// シェーダー設定
	const uint32_t MaxPayloadSize = sizeof(float) * 3 + sizeof(uint32_t);
	const uint32_t MaxAttributeSize = sizeof(float) * 2;
	stateObjectBuilder_.SetShaderConfig(MaxPayloadSize, MaxAttributeSize);
	stateObjectBuilder_.SetPipelineConfig(3);

	// グローバルルートシグネチャを設定
	stateObjectBuilder_.SetGlobalRootSignature(rootSignatureGlobal_.Get());

	// ローカルルートシグネチャとシェーダーを関連づける
	stateObjectBuilder_.AddLocalRootSignature(rsRayGen_.Get(), { L"MainRayGen" });
	stateObjectBuilder_.AddLocalRootSignature(rsModel_.Get(), { AppHitGroups::DefaultModel });

	// 生成する
	stateObject_ = stateObjectBuilder_.Build(device_);
}

void RaytracingPipeline::CreateShaderTable(ModelManager* modelManager, GpuResource* cameraResource, GpuResource* lightResource) {

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
		table.AppendDescriptor(tlas_->GetSrvHandleGPU());
		table.AppendDescriptor(srvManager_->GetGPUHandle(renderPassController_->GetUavIndex("RaytracingPass")));
		table.AppendGPUAddress(cameraResource->GetGpuVirtualAddress());
		table.AppendGPUAddress(lightResource->GetGpuVirtualAddress());
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
	}

	// ヒットグループ番号
	uint32_t hitGroupIndex = 0;
	// hitGroup
	{
		auto& models = modelManager->GetModels();

		auto id = rtsoProps->GetShaderIdentifier(AppHitGroups::DefaultModel.c_str());
		if (id == nullptr) {
			assert(false && "Not found ShaderIdentifier");
		}

		for (auto& [key, data] : models) {
			for (auto& mesh : data.model->GetMeshes()) {
				mesh->SetHitGroupIndex(hitGroupIndex);
				auto& index = mesh->GetIndexBuffer();
				auto& vertex = mesh->GetVertexBuffer();

				ShaderRecord record;
				auto table = record.SetIdentifier(id);
				table.AppendDescriptor(index.GetSrvGpuHandle());
				table.AppendDescriptor(vertex.GetSrvGpuHandle());

				shaderTableBuilder_.HitGroup().AddRecord(std::move(record));

				hitGroupIndex++;
			}
		}
	}

	// テーブルを設定する
	shaderTableBuilder_.Build(device_);

	// 保存する
	dispatchRayDesc_ = shaderTableBuilder_.CreateDispatchRaysDesc(1280, 720);
}
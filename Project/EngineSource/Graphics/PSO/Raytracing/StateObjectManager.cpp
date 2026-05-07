#include "StateObjectManager.h"
#include "ModelManager.h"
#include "EngineSource/Graphics/PSO/Core/RootSignatureBuilder.h"
using namespace GameEngine;

void StateObjectManager::Initialize(ID3D12Device5* device, SrvManager* srvManager,DXC* dxc,
	RenderPassController* renderPassController, ModelManager* modelManager, TLAS* tlas) {
	device_ = device;
	srvManager_ = srvManager;
	renderPassController_ = renderPassController;
	modelManager_ = modelManager;
	tlas_ = tlas;

	// シェーダコンパイル機能を初期化
	rayLibShaderCompiler_.Initialize(dxc);

	// ルートシグネチャを作成する
	CreateGlobalRootsignature();
	CreateLocalRootsignature();
}

void StateObjectManager::Create() {

	// ステートオブジェクトを作成する
	CreateStateObject();

	// シェーダーテーブルを設定する
	CreateShaderTable();
}

void StateObjectManager::CreateGlobalRootsignature() {

	// tlasの設定、カメラ、ライトの設定、マテリアルアクセスデータ、バッファデータの設定

	RootSignatureBuilder builder;
	builder.Initialize(device_);
	builder.AddSRVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_ALL); // tlas
	builder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 1, D3D12_SHADER_VISIBILITY_ALL); // テクスチャ
	builder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::AccessMaxCount), 2, D3D12_SHADER_VISIBILITY_ALL); // アクセスデータ
	builder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::BufferMaxCount), 3, D3D12_SHADER_VISIBILITY_ALL); // マテリアルなどのデータ
	builder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL); // light
	builder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL); // camera
	builder.CreateRootSignature();
	rootSignatureGlobal_ = builder.MoveOwnerRootSignature();
}

void StateObjectManager::CreateLocalRootsignature() {

	// raygen用
	RootSignatureBuilder raygenBuilder;
	raygenBuilder.Initialize(device_);
	raygenBuilder.AddUAVDescriptorTable(0, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
	raygenBuilder.CreateRootSignature();
	rsRayGen_ = raygenBuilder.MoveOwnerRootSignature();

	// Object用
	RootSignatureBuilder objectBuilder;
	objectBuilder.Initialize(device_);
	objectBuilder.AddSRVDescriptorTable(0, 1, 1, D3D12_SHADER_VISIBILITY_ALL);
	objectBuilder.AddSRVDescriptorTable(1, 1, 1, D3D12_SHADER_VISIBILITY_ALL);
	objectBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_ALL);
	objectBuilder.CreateRootSignature();
	rsModel_ = objectBuilder.MoveOwnerRootSignature();
}

void StateObjectManager::CreateStateObject() {
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
	stateObjectBuilder_.SetPipelineConfig(2);

	// グローバルルートシグネチャを設定
	stateObjectBuilder_.SetGlobalRootSignature(rootSignatureGlobal_.Get());

	// ローカルルートシグネチャとシェーダーを関連づける
	stateObjectBuilder_.AddLocalRootSignature(rsRayGen_.Get(), { L"MainRayGen" });
	stateObjectBuilder_.AddLocalRootSignature(rsModel_.Get(), { AppHitGroups::DefaultModel });

	// 生成する
	Microsoft::WRL::ComPtr<ID3D12StateObject> stateObject = stateObjectBuilder_.Build(device_);
}

void StateObjectManager::CreateShaderTable() {

	// raygen
	{
		ShaderRecord record;
		auto table = record.SetIdentifier(L"MainRayGen");
		table.AppendDescriptor(tlas_->GetSrvHandleGPU());
		table.AppendDescriptor(srvManager_->GetGPUHandle(renderPassController_->GetUavIndex("RaytracingPass")));
		shaderTableBuilder_.RayGen().AddRecord(std::move(record));
	}

	// miss
	{
		ShaderRecord record;
		record.SetIdentifier(L"MainMiss");
		shaderTableBuilder_.Miss().AddRecord(std::move(record));
	}

	// ヒットグループ番号
	uint32_t hitGroupIndex = 0;
	// hitGroup
	{
		auto& models = modelManager_->GetModels();

		for (auto& [key, data] : models) {
			for (auto& mesh : data.model->GetMeshes()) {
				data.model->SetHitGroupIndex(hitGroupIndex);
				auto& index = mesh->GetIndexBuffer();
				auto& vertex = mesh->GetVertexBuffer();

				ShaderRecord record;
				auto table = record.SetIdentifier(AppHitGroups::DefaultModel.c_str());
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
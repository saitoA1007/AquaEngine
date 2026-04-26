#include "TestManager.h"
#include <cassert>
#include "CreateBufferResource.h"
#include "ResourceGarbageCollector.h"
#include <algorithm>
#include <random>
using namespace GameEngine;

void TestManager::Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList, DXC* dxc, SrvManager* srvManager, RenderPassController* renderPassController,
    TestCamera* testCamera, TextureManager* textureManager) {
	device_ = device;
	commandList_ = commandList;
	dxc_ = dxc;
    srvManager_ = srvManager;
	renderPassController_ = renderPassController;
    testCamera_ = testCamera;
    textureManager_ = textureManager;

    // 床と球のオブジェクトを生成する.
    CreateSceneObjects();
    // 床とキューブの BLAS を構築する.
    CreateSceneBLAS();
    // シーンに配置して TLAS を構築する.
    CreateSceneTLAS();
    // グローバル Root Signature を用意.
    CreateRootSignatureGlobal();
    // ローカル Root Signature を用意.
    CreateLocalRootSignatureRayGen();
    CreateSphereLocalRootSignature();
    CreateFloorLocalRootSignature();
    // コンパイル済みシェーダーよりステートオブジェクトを用意.
    CreateStateObject();
    // レイトレーシング結果格納のためのバッファ(UAV)を用意.
    CreateResultBuffer();
    // 描画で使用する Shader Table を用意.
    CreateShaderTable();
}

void TestManager::Update() {


}

void TestManager::Draw() {

    renderPassController_->PrePass("RaytracingPass");

    commandList_->SetComputeRootSignature(rootSignatureGlobal_.Get());
    // TLASのセット
    commandList_->SetComputeRootDescriptorTable(0, srvManager_->GetGPUHandle(tlasSrvIndex_));
    // カメラのセット
    commandList_->SetComputeRootConstantBufferView(1, testCamera_->constBuffer_.GetGpuVirtualAddress());
    // 背景画像(dds)をセット
    commandList_->SetComputeRootDescriptorTable(2, srvManager_->GetGPUHandle(textureManager_->GetHandleByName("rostock_laage_airport_4k.dds")));

    // レイトレーシングを開始.
    commandList_->SetPipelineState1(stateObject_.Get());
    commandList_->DispatchRays(&dispatchRayDesc_);

    // UAV書き込み完了
    renderPassController_->InsertUavBarrier("RaytracingPass");
    // SRV状態へ
    renderPassController_->PostPass("RaytracingPass");
}

void TestManager::CreateSceneObjects() {
    //==================================================
    // 床の生成
    //==================================================
    std::vector<VertexPN> verticesPN;
    std::vector<uint32_t> indices;
    GetPlane(verticesPN, indices);
    meshPlane.indexBuffer_.Create(commandList_, indices);
    meshPlane.vertexBuffer_.Create(commandList_, verticesPN);
    meshPlane.shaderName = AppHitGroups::Floor.c_str();

    //========================================================
    // スフィアの生成.
    //========================================================
    indices.clear();
    verticesPN.clear();
    GetSphere(verticesPN, indices, 0.5f, 32, 48);
    meshSphere_.indexBuffer_.Create(commandList_, indices);
    meshSphere_.vertexBuffer_.Create(commandList_, verticesPN);
    meshSphere_.shaderName = AppHitGroups::Sphere.c_str();

    //=======================================================
    // ライト用スフィアを作成
    //=======================================================
    indices.clear();
    verticesPN.clear();
    GetSphere(verticesPN, indices, 1.0f, 32, 48);
    meshLightSphere_.indexBuffer_.Create(commandList_, indices);
    meshLightSphere_.vertexBuffer_.Create(commandList_, verticesPN);
    meshLightSphere_.shaderName = AppHitGroups::Light.c_str();

    //// スフィアを適当に配置する.
    std::mt19937 mt;
    std::uniform_real_distribution rnd(.5f, 0.75f);
    std::uniform_int_distribution rnd2(-9, 9);
    for (auto& sphere : spheres_) {
        float x = rnd2(mt) + 0.5f;
        float z = rnd2(mt) + 0.5f;
        sphere.mtxWorld = MakeTranslateMatrix(Vector3(x, 0.5, z));
    }

    // ポイントライト位置用.
    pointLight_.mtxWorld = MakeTranslateMatrix(Vector3(0, 2.5, 2));
}

void TestManager::CreateSceneBLAS() {
   
    // 床のBLASを作成する
    auto& vp = meshPlane.vertexBuffer_;
    auto& ip = meshPlane.indexBuffer_;
    meshPlane.blas_.Create(commandList_, vp.GetView(), ip.GetView(), vp.GetTotalVertices(), ip.GetTotalIndices());

    // cubeのBLASを作成する
    auto& vc = meshSphere_.vertexBuffer_;
    auto& ic = meshSphere_.indexBuffer_;
    meshSphere_.blas_.Create(commandList_,vc.GetView(),ic.GetView(),vc.GetTotalVertices(),ic.GetTotalIndices());

    // ライト用SphereのBLASを作成する
    auto& vcl = meshLightSphere_.vertexBuffer_;
    auto& icl = meshLightSphere_.indexBuffer_;
    meshLightSphere_.blas_.Create(commandList_, vcl.GetView(), icl.GetView(), vcl.GetTotalVertices(), icl.GetTotalIndices());
}

void TestManager::CreateSceneTLAS() {

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    DeployObjects(instanceDescs);

    // GPUに送るためのリソースを作成
    instanceDescBuffer_ = CreateBufferResource(
        device_, instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_FLAG_NONE
    );

    D3D12_RAYTRACING_INSTANCE_DESC* pDesc = nullptr;
    instanceDescBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&pDesc));
    std::memcpy(pDesc, instanceDescs.data(), instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
    instanceDescBuffer_->Unmap(0, nullptr);
    pDesc = nullptr;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
    auto& inputs = asDesc.Inputs; // D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

    // 数を指定
    inputs.NumDescs = UINT(instanceDescs.size());
    inputs.InstanceDescs = instanceDescBuffer_->GetGPUVirtualAddress();

#pragma region resource
    // 必要なメモリ量を求める.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuild{};
    device_->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &tlasPrebuild);

    // 一時的な作業リソースを作成
    Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer = CreateBufferResource(
        device_, tlasPrebuild.ScratchDataSizeInBytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    // TLASのデータリソースを作成
    tlas_ = CreateBufferResource(
        device_, tlasPrebuild.ResultDataMaxSizeInBytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    if(tlas_ == nullptr || scratchBuffer == nullptr) {
        throw std::runtime_error("TLAS Creation failed.");
    }
    tlas_->SetName(L"Scene-Tlas");

    // アップデート用バッファを確保
    if (asDesc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE) {

        asbUpdate = CreateBufferResource(
            device_, tlasPrebuild.UpdateScratchDataSizeInBytes,
            D3D12_HEAP_TYPE_DEFAULT,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        );
        if (asbUpdate == nullptr) {
            throw std::runtime_error("CreateAccelerationStructure failed.");
        }
    }

    // Acceleration Structure 構築.
    asDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
    asDesc.DestAccelerationStructureData = tlas_->GetGPUVirtualAddress();
    commandList_->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

    // バリア生成
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = tlas_.Get();
    commandList_->ResourceBarrier(1, &barrier);

    // リソースの破棄を登録する
    ResourceGarbageCollector::GetInstance().Add(scratchBuffer);
#pragma endregion

    // SRVを作成
    tlasSrvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = tlas_->GetGPUVirtualAddress();

    auto srvHandleCPU = srvManager_->GetCPUHandle(tlasSrvIndex_);
    device_->CreateShaderResourceView(nullptr, &srvDesc, srvHandleCPU);
}

//=============================================
// 
// ルートシグネチャ(Global) を生成します.
// 
//=============================================
void TestManager::CreateRootSignatureGlobal() {
    std::array<CD3DX12_ROOT_PARAMETER, 3> rootParams{};

    // TLAS を t0 レジスタに割り当てて使用する設定.
    CD3DX12_DESCRIPTOR_RANGE descRangeTLAS;
    descRangeTLAS.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    // 背景用テクスチャを t1
    CD3DX12_DESCRIPTOR_RANGE descRangeBgGh;
    descRangeBgGh.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

    rootParams[0].InitAsDescriptorTable(1, &descRangeTLAS); // t0
    rootParams[1].InitAsConstantBufferView(0); // b0
    rootParams[2].InitAsDescriptorTable(1, &descRangeBgGh); // t1

    // サンプラーの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMIpmapを使う
    staticSamplers[0].ShaderRegister = 0; // レジスタ番号0を使う
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
    // パラメータを設定
    rootSigDesc.NumParameters = UINT(rootParams.size());
    rootSigDesc.pParameters = rootParams.data();
    // サンプラーを設定
    rootSigDesc.pStaticSamplers = staticSamplers;
    rootSigDesc.NumStaticSamplers = _countof(staticSamplers);

    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob, errBlob;
    hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errBlob);
    if (FAILED(hr)) {
        throw std::runtime_error("RootSignature(global) failed.");
    }

    hr = device_->CreateRootSignature(
        0, blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(rootSignatureGlobal_.ReleaseAndGetAddressOf())
    );
    if (FAILED(hr)) {
        throw std::runtime_error("RootSignature(global) failed.");
    }
    rootSignatureGlobal_->SetName(L"RootSignatureGlobal");
}

//=============================================
// 
// ルートシグネチャ(Local) を生成します.
// 
//=============================================
void TestManager::CreateLocalRootSignatureRayGen() {

    // UAV (u0)
    D3D12_DESCRIPTOR_RANGE descUAV{};
    descUAV.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descUAV.BaseShaderRegister = 0;
    descUAV.NumDescriptors = 1;

    std::array<D3D12_ROOT_PARAMETER, 1> rootParams;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[0].DescriptorTable.pDescriptorRanges = &descUAV;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = UINT(rootParams.size());
    rootSigDesc.pParameters = rootParams.data();
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob, errBlob;
    hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errBlob);
    if (FAILED(hr)) {
        throw std::runtime_error("RootSignature(lrsRayGen) failed.");
    }

    hr = device_->CreateRootSignature(
        0, blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(rsRGS.ReleaseAndGetAddressOf())
    );
    if (FAILED(hr)) {
        throw std::runtime_error("RootSignature(lrsRayGen) failed.");
    }
    rsRGS->SetName(L"lrsRayGen");
}

void TestManager::CreateSphereLocalRootSignature() {
    // 頂点・インデックスバッファにアクセスするためのローカルルートシグネチャを作る.
    D3D12_DESCRIPTOR_RANGE rangeIB{};
    rangeIB.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    rangeIB.BaseShaderRegister = 0;
    rangeIB.NumDescriptors = 1;
    rangeIB.RegisterSpace = 1;

    D3D12_DESCRIPTOR_RANGE rangeVB{};
    rangeVB.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    rangeVB.BaseShaderRegister = 1;
    rangeVB.NumDescriptors = 1;
    rangeVB.RegisterSpace = 1;

    {
        std::array<D3D12_ROOT_PARAMETER, 2> rootParams;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[0].DescriptorTable.pDescriptorRanges = &rangeIB;

        rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[1].DescriptorTable.pDescriptorRanges = &rangeVB;

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
        rootSigDesc.NumParameters = UINT(rootParams.size());
        rootSigDesc.pParameters = rootParams.data();
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        HRESULT hr;
        Microsoft::WRL::ComPtr<ID3DBlob> blob, errBlob;
        hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errBlob);
        if (FAILED(hr)) {
            throw std::runtime_error("RootSignature(rsSphere1) failed.");
        }

        hr = device_->CreateRootSignature(
            0, blob->GetBufferPointer(), blob->GetBufferSize(),
            IID_PPV_ARGS(rsSphere1_.ReleaseAndGetAddressOf())
        );
        if (FAILED(hr)) {
            throw std::runtime_error("RootSignature(rsSphere1) failed.");
        }
        rsSphere1_->SetName(L"rsSphere1");
    } 

    //========================================================================
    // 反射、屈折用
    //========================================================================

    {
        std::array<D3D12_ROOT_PARAMETER, 3> rootParams;
        rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[0].DescriptorTable.pDescriptorRanges = &rangeIB; // t0

        rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
        rootParams[1].DescriptorTable.pDescriptorRanges = &rangeVB; // t1

        rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParams[2].Descriptor.ShaderRegister = 0;  // b0
        rootParams[2].Descriptor.RegisterSpace = 1;

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
        rootSigDesc.NumParameters = UINT(rootParams.size());
        rootSigDesc.pParameters = rootParams.data();
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        HRESULT hr;
        Microsoft::WRL::ComPtr<ID3DBlob> blob, errBlob;
        hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errBlob);
        if (FAILED(hr)) {
            throw std::runtime_error("RootSignature(rsSphere2) failed.");
        }

        hr = device_->CreateRootSignature(
            0, blob->GetBufferPointer(), blob->GetBufferSize(),
            IID_PPV_ARGS(rsSphere2_.ReleaseAndGetAddressOf())
        );
        if (FAILED(hr)) {
            throw std::runtime_error("RootSignature(rsSphere2) failed.");
        }
        rsSphere2_->SetName(L"rsSphere2");
    }
}

void TestManager::CreateFloorLocalRootSignature() {
    // 頂点・インデックスバッファにアクセスするためのローカルルートシグネチャを作る.
    D3D12_DESCRIPTOR_RANGE rangeIB{};
    rangeIB.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    rangeIB.BaseShaderRegister = 0;
    rangeIB.NumDescriptors = 1;
    rangeIB.RegisterSpace = 1;

    D3D12_DESCRIPTOR_RANGE rangeVB{};
    rangeVB.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    rangeVB.BaseShaderRegister = 1;
    rangeVB.NumDescriptors = 1;
    rangeVB.RegisterSpace = 1;

    D3D12_DESCRIPTOR_RANGE rangeTex{};
    rangeTex.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    rangeTex.BaseShaderRegister = 2;
    rangeTex.NumDescriptors = 1;
    rangeTex.RegisterSpace = 1;

    std::array<D3D12_ROOT_PARAMETER, 3> rootParams;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[0].DescriptorTable.pDescriptorRanges = &rangeIB;

    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &rangeVB;

    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[2].DescriptorTable.pDescriptorRanges = &rangeTex;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = UINT(rootParams.size());
    rootSigDesc.pParameters = rootParams.data();
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    HRESULT hr;
    Microsoft::WRL::ComPtr<ID3DBlob> blob, errBlob;
    hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errBlob);
    if (FAILED(hr)) {
        throw std::runtime_error("RootSignature(rsFloor) failed.");
    }

    hr = device_->CreateRootSignature(
        0, blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(rsFloor_.ReleaseAndGetAddressOf())
    );
    if (FAILED(hr)) {
        throw std::runtime_error("RootSignature(rsFloor) failed.");
    }
    rsFloor_->SetName(L"rsFloor");
}

//============================================================================
// 
// レイトレーシング用の StateObject を構築します.
// 
//============================================================================
void TestManager::CreateStateObject() {
    CD3DX12_STATE_OBJECT_DESC rtPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    //===================================================================================
    // シェーダーファイルの読み込み
    //===================================================================================
    auto chsFloorBlob = dxc_->CompileShader(L"Resources/Shaders/Raytracing/chsFloor.hlsl", L"lib_6_5", L"");
    auto chsSphereNoLambertBlob = dxc_->CompileShader(L"Resources/Shaders/Raytracing/chsSphereNoLambert.hlsl", L"lib_6_5", L"");
    auto chsSpherePhongBlob = dxc_->CompileShader(L"Resources/Shaders/Raytracing/chsSpherePhong.hlsl", L"lib_6_5", L"");
    auto missBlob = dxc_->CompileShader(L"Resources/Shaders/Raytracing/miss.hlsl", L"lib_6_5", L"");
    auto raygenBlob = dxc_->CompileShader(L"Resources/Shaders/Raytracing/raygen.hlsl", L"lib_6_5", L"");

    //===================================================================================
    // シェーダーから各関数レコード
    //===================================================================================
    
    //レイを飛ばす
    auto dxilRayGen = rtPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE rayGenlibdxil = { raygenBlob->GetBufferPointer(), raygenBlob->GetBufferSize() };
    dxilRayGen->SetDXILLibrary(&rayGenlibdxil);
    dxilRayGen->DefineExport(L"mainRayGen");

    // レイに当たらなかった場合
    auto dxilMiss = rtPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE misslibdxil = { missBlob->GetBufferPointer(), missBlob->GetBufferSize() };
    dxilMiss->SetDXILLibrary(&misslibdxil);
    dxilMiss->DefineExport(L"mainMiss");

    // レイがオブジェクトに当たった時
    auto dxilChsFloor = rtPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE floorlibdxil = { chsFloorBlob->GetBufferPointer(), chsFloorBlob->GetBufferSize() };
    dxilChsFloor->SetDXILLibrary(&floorlibdxil);
    dxilChsFloor->DefineExport(L"mainFloorCHS");

    auto dxilChsSphere0 = rtPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE chsSphereNoLambertLibdxil = { chsSphereNoLambertBlob->GetBufferPointer(), chsSphereNoLambertBlob->GetBufferSize() };
    dxilChsSphere0->SetDXILLibrary(&chsSphereNoLambertLibdxil);
    dxilChsSphere0->DefineExport(L"chsSphereMaterial");

    auto dxilChsSphere1 = rtPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE chsSpherePhongLibdxil = { chsSpherePhongBlob->GetBufferPointer(), chsSpherePhongBlob->GetBufferSize() };
    dxilChsSphere1->SetDXILLibrary(&chsSpherePhongLibdxil);
    dxilChsSphere1->DefineExport(L"chsSphereMaterialPhong");

    //===================================================================================
    // ヒットグループの設定 
    //===================================================================================

    // ヒットグループの設定(球体に対する).
    auto hitgroupPhong = rtPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitgroupPhong->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
    hitgroupPhong->SetClosestHitShaderImport(L"chsSphereMaterialPhong");
    hitgroupPhong->SetHitGroupExport(AppHitGroups::PhongMaterial.c_str());

    // ヒットグループの設定(反射および屈折の球体に対する).
    auto hitgroupNoPhong = rtPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitgroupNoPhong->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
    hitgroupNoPhong->SetClosestHitShaderImport(L"chsSphereMaterial");
    hitgroupNoPhong->SetHitGroupExport(AppHitGroups::NoPhongMaterial.c_str());

    // ヒットグループの設定(床に対する).
    auto hitgroupFloor = rtPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitgroupFloor->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
    hitgroupFloor->SetClosestHitShaderImport(L"mainFloorCHS");
    hitgroupFloor->SetHitGroupExport(AppHitGroups::Floor.c_str());

    //===================================================================================
    // グローバルルートシグネチャの設定
    //===================================================================================
    auto globalRootSignature = rtPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(rootSignatureGlobal_.Get());

    //===================================================================================
    // ローカルルートシグネチャの設定
    //===================================================================================

    // レイの発射
    auto rsRayGen = rtPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    rsRayGen->SetRootSignature(rsRGS.Get());
    auto lrsAssocRGS = rtPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    lrsAssocRGS->AddExport(L"mainRayGen");
    lrsAssocRGS->SetSubobjectToAssociate(*rsRayGen);

    // 床用.
    auto rsFloor = rtPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    rsFloor->SetRootSignature(rsFloor_.Get());
    auto lrsAssocFloor = rtPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    lrsAssocFloor->AddExport(AppHitGroups::Floor.c_str());
    lrsAssocFloor->SetSubobjectToAssociate(*rsFloor);

    // 非Phongの球体に対して.
    auto rsModel = rtPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    rsModel->SetRootSignature(rsSphere1_.Get());
    auto lrsAssocModel = rtPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    lrsAssocModel->AddExport(AppHitGroups::NoPhongMaterial.c_str());
    lrsAssocModel->SetSubobjectToAssociate(*rsModel);

    // Phongの球体に対して.
    auto rsPhongSphere = rtPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    rsPhongSphere->SetRootSignature(rsSphere2_.Get());
    auto lrsAssocPhongSphere = rtPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    lrsAssocPhongSphere->AddExport(AppHitGroups::PhongMaterial.c_str());
    lrsAssocPhongSphere->SetSubobjectToAssociate(*rsPhongSphere);

    //===================================================================================
    // シェーダーの設定
    //===================================================================================
    const uint32_t MaxPayloadSize = sizeof(float) * 3 + sizeof(uint32_t);
    const uint32_t MaxAttributeSize = sizeof(float) * 2;

    auto shaderConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shaderConfig->Config(MaxPayloadSize, MaxAttributeSize);

    //=================================================================================== 
    // パイプラインの設定 レイの再帰可能段数
    //===================================================================================
    const uint32_t MaxRecursionDepth = 16;
    auto pipelineConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    // 1にすると「カメラからのレイ」のみ。反射（鏡など）を作るなら 2 や 3 にする
    pipelineConfig->Config(MaxRecursionDepth);

    //===================================================================================
    // ステートオブジェクトの生成
    //===================================================================================
    HRESULT hr = device_->CreateStateObject(rtPipeline, IID_PPV_ARGS(&stateObject_));
    assert(SUCCEEDED(hr));
}

//==========================================================
//
// レイトレーシング結果書き込み用バッファを生成します.
// 
//==========================================================
void TestManager::CreateResultBuffer() {

    // ここではレイトレーシング用で書き込むためのテクスチャを用意する
    // すでに描画用のテクスチャを作成するクラスが存在しているのでそれを利用する

    // 描画するパス
    renderPassController_->AddPass("RaytracingPass", RenderTextureMode::UavOnly);
}

//==========================================================
//
// シェーダーテーブルを作成
// 
//==========================================================
// レイトレーシングで使用する ShaderTable を構築します.
void TestManager::CreateShaderTable() {

   //==============================================
   // シェーダーレコードのサイズとテーブルのサイズを計算する
   //==============================================
#pragma region シェーダーレコードのサイズとテーブルのサイズを計算する
    const auto ShaderRecordAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
    // RayGeneration シェーダーでは、 Shader Identifier と
    // ローカルルートシグネチャによる u0 のディスクリプタを使用.
    UINT raygenRecordSize = 0;
    raygenRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    raygenRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
    raygenRecordSize = RoundUp(raygenRecordSize, ShaderRecordAlignment);

    // ヒットグループでは、 Shader Identifier の他に
    // ローカルルートシグネチャによる VB/IB のディスクリプタを使用.
    UINT hitgroupRecordSize = 0;
    hitgroupRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
    hitgroupRecordSize += sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
    hitgroupRecordSize = RoundUp(hitgroupRecordSize, ShaderRecordAlignment);

    // Missシェーダーではローカルルートシグネチャ未使用.
    UINT missRecordSize = 0;
    missRecordSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    missRecordSize = RoundUp(missRecordSize, ShaderRecordAlignment);

    // 使用する各シェーダーの個数より、シェーダーテーブルのサイズを求める.
    UINT raygenSize = 1 * raygenRecordSize; // 今1つの Ray Generation シェーダー.
    UINT missSize = 1 * missRecordSize;  // 今1つの Miss シェーダー.
    UINT hitgroupCount = 1 + 1 + NormalSpheres;// (床/反射・屈折共用/Phong描画用) によるヒットグループ.
    UINT hitGroupSize = hitgroupCount * hitgroupRecordSize;

    // 各テーブルの開始位置にアライメント制約があるので調整する.
    auto tableAlign = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    auto raygenRegion = RoundUp(raygenSize, tableAlign);
    auto missRegion = RoundUp(missSize, tableAlign);
    auto hitgroupRegion = RoundUp(hitGroupSize, tableAlign);
#pragma endregion


   //==============================================
   // シェーダーレコードのサイズとテーブルのサイズを計算する
   //==============================================
#pragma region シェーダーテーブルの構築
    // シェーダーテーブル確保.
    auto tableSize = raygenRegion + missRegion + hitgroupRegion;
    CreateBuffer(tableSize);

    Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtsoProps;
    stateObject_.As(&rtsoProps);

    // 各シェーダーレコードを書き込んでいく.
    void* mapped = nullptr;
    shaderTable_->Map(0, nullptr, &mapped);
    uint8_t* pStart = static_cast<uint8_t*>(mapped);

    // RayGeneration 用のシェーダーレコードを書き込み.
    auto rgsStart = pStart;
    {
        uint8_t* p = rgsStart;
        auto id = rtsoProps->GetShaderIdentifier(L"mainRayGen");
        if (id == nullptr) {
            throw std::logic_error("Not found ShaderIdentifier");
        }
        p += WriteShaderIdentifier(p, id);

        // ローカルルートシグネチャで u0 (出力先) を設定しているため
        // 対応するディスクリプタを書き込む.
        auto outputHandle = srvManager_->GetGPUHandle(renderPassController_->GetUavIndex("RaytracingPass"));
        p += WriteGPUDescriptor(p, outputHandle);
    }

    // Miss Shader 用のシェーダーレコードを書き込み.
    auto missStart = pStart + raygenRegion;
    {
        uint8_t* p = missStart;
        auto id = rtsoProps->GetShaderIdentifier(L"mainMiss");
        if (id == nullptr) {
            throw std::logic_error("Not found ShaderIdentifier");
        }
        p += WriteShaderIdentifier(p, id);

        // ローカルルートシグネチャ使用時には他のデータを書き込む.
    }

    // Hit Group 用のシェーダーレコードを書き込み.
    auto hitgroupStart = pStart + raygenRegion + missRegion;
    {
        auto recordStart = hitgroupStart;

        auto id = rtsoProps->GetShaderIdentifier(L"hgFloor");
        if (id == nullptr) {
            throw std::logic_error("Not found ShaderIdentifier");
        }
        // plane に対応するシェーダーエントリを書き込む.
        recordStart = WriteHitgroupFloor(recordStart, meshPlane, hitgroupRecordSize);

        // sphere に対応するシェーダーエントリを書き込む.
        id = rtsoProps->GetShaderIdentifier(L"hgNoPhongSpheres");
        if (id == nullptr) {
            throw std::logic_error("Not found ShaderIdentifier");
        }
        recordStart = WriteHitgroupMaterial(recordStart, meshSphere_, hitgroupRecordSize);

        // sphere (定数バッファ使用) に対応するエントリを書き込む.
        id = rtsoProps->GetShaderIdentifier(L"hgPhongSpheres");

        // マテリアル情報を格納した定数バッファのアドレス位置をずらし別のマテリアルを参照させる.
        auto cbAddress = normalSphereMaterialCB_->GetGPUVirtualAddress();
        auto stride = sizeof(MaterialParam);
        for (auto& sphere : spheresNormal_) {
            recordStart = WriteHitgroupPhong(recordStart, meshSphere_, cbAddress, hitgroupRecordSize);
            cbAddress += stride;
        }
    }

    shaderTable_->Unmap(0, nullptr);

    // DispatchRays のために情報をセットしておく.
    auto startAddress = shaderTable_->GetGPUVirtualAddress();

    auto& shaderRecordRG = dispatchRayDesc_.RayGenerationShaderRecord;
    shaderRecordRG.StartAddress = startAddress;
    shaderRecordRG.SizeInBytes = raygenSize;
    startAddress += raygenRegion;

    auto& shaderRecordMS = dispatchRayDesc_.MissShaderTable;
    shaderRecordMS.StartAddress = startAddress;
    shaderRecordMS.SizeInBytes = missSize;
    shaderRecordMS.StrideInBytes = missRecordSize;
    startAddress += missRegion;

    auto& shaderRecordHG = dispatchRayDesc_.HitGroupTable;
    shaderRecordHG.StartAddress = startAddress;
    shaderRecordHG.SizeInBytes = hitGroupSize;
    shaderRecordHG.StrideInBytes = hitgroupRecordSize;
    startAddress += hitgroupRegion;

    dispatchRayDesc_.Width = 1280;
    dispatchRayDesc_.Height = 720;
    dispatchRayDesc_.Depth = 1;
#pragma endregion
}

//===========================================================================
// 
// ヘルプ関数
// 
//===========================================================================

void TestManager::DeployObjects(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instanceDescs) {
    // 床を配置
    {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        Matrix4x4 matrix = Transpose(MakeIdentity4x4());
        std::memcpy(&desc.Transform, &matrix, sizeof(float) * 12);
        desc.InstanceID = 0;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = 0;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = meshPlane.blas_.GetGpuVirtualAddress();
        instanceDescs.push_back(desc);
    }

    // ライトを配置
    {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        Matrix4x4 matrix = Transpose(pointLight_.mtxWorld);
        std::memcpy(&desc.Transform, &matrix, sizeof(float) * 12);
        desc.InstanceID = 0;
        desc.InstanceMask = 0x08;
        desc.InstanceContributionToHitGroupIndex = 1;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = meshLightSphere_.blas_.GetGpuVirtualAddress();
        instanceDescs.push_back(desc);
    }

    // スフィアを配置
    uint32_t instanceID = 0;
    for (const auto& sphere : spheres_) {
        D3D12_RAYTRACING_INSTANCE_DESC desc{};
        Matrix4x4 matrix = Transpose(sphere.mtxWorld);
        std::memcpy(&desc.Transform, &matrix, sizeof(float) * 12);
        desc.InstanceID = instanceID;
        desc.InstanceMask = 0xFF;
        desc.InstanceContributionToHitGroupIndex = 2;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = meshSphere_.blas_.GetGpuVirtualAddress();
        instanceDescs.push_back(desc);
        instanceID++;
    }
}

void TestManager::GetPlane(std::vector<VertexPN>& vertices, std::vector<UINT>& indices) {
    float size = 10.0f;
    VertexPN srcVertices[] = {
         VertexPN{ {-1.0f, 0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }},
         VertexPN{ {-1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }},
         VertexPN{ { 1.0f, 0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }},
         VertexPN{ { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }},
    };
    vertices.resize(4);
    std::transform(
        std::begin(srcVertices), std::end(srcVertices), vertices.begin(),
        [=](auto v) {
            v.Position.x *= size;
            v.Position.z *= size;
            return v;
        }
    );
    indices = { 0, 1, 2, 2, 1, 3 };
}

void TestManager::GetColoredCube(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices) {
    float size = 1.0f;

    vertices.clear();
    indices.clear();

    const Vector4 red(1.0f, 0.0f, 0.0f, 1.0f);
    const Vector4 green(0.0f, 1.0f, 0.0f, 1.0f);
    const Vector4 blue(0.0f, 0.0f, 1.0f, 1.0);
    const Vector4 white(1.0f, 1.0f, 1.0f, 1.0f);
    const Vector4 black(0.0f, 0.0f, 0.0f, 1.0f);
    const Vector4 yellow(1.0f, 1.0f, 0.0f, 1.0f);
    const Vector4 magenta(1.0f, 0.0f, 1.0f, 1.0f);
    const Vector4 cyan(0.0f, 1.0f, 1.0f, 1.0f);

    vertices = {
        // 裏
        { {-1.0f,-1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f }, red },
        { {-1.0f, 1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f }, yellow },
        { { 1.0f, 1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f }, white },
        { { 1.0f,-1.0f,-1.0f}, { 0.0f, 0.0f, -1.0f }, magenta },
        // 右
        { { 1.0f,-1.0f,-1.0f}, { 1.0f, 0.0f, 0.0f }, magenta },
        { { 1.0f, 1.0f,-1.0f}, { 1.0f, 0.0f, 0.0f }, white},
        { { 1.0f, 1.0f, 1.0f}, { 1.0f, 0.0f, 0.0f }, cyan},
        { { 1.0f,-1.0f, 1.0f}, { 1.0f, 0.0f, 0.0f }, blue},
        // 左
        { {-1.0f,-1.0f, 1.0f}, { -1.0f, 0.0f, 0.0f }, black},
        { {-1.0f, 1.0f, 1.0f}, { -1.0f, 0.0f, 0.0f }, green},
        { {-1.0f, 1.0f,-1.0f}, { -1.0f, 0.0f, 0.0f }, yellow},
        { {-1.0f,-1.0f,-1.0f}, { -1.0f, 0.0f, 0.0f }, red},
        // 正面
        { { 1.0f,-1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, blue},
        { { 1.0f, 1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, cyan},
        { {-1.0f, 1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, green},
        { {-1.0f,-1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, black},
        // 上
        { {-1.0f, 1.0f,-1.0f}, { 0.0f, 1.0f, 0.0f}, yellow},
        { {-1.0f, 1.0f, 1.0f}, { 0.0f, 1.0f, 0.0f}, green },
        { { 1.0f, 1.0f, 1.0f}, { 0.0f, 1.0f, 0.0f}, cyan },
        { { 1.0f, 1.0f,-1.0f}, { 0.0f, 1.0f, 0.0f}, white},
        // 底
        { {-1.0f,-1.0f, 1.0f}, { 0.0f, -1.0f, 0.0f}, black},
        { {-1.0f,-1.0f,-1.0f}, { 0.0f, -1.0f, 0.0f}, red},
        { { 1.0f,-1.0f,-1.0f}, { 0.0f, -1.0f, 0.0f}, magenta},
        { { 1.0f,-1.0f, 1.0f}, { 0.0f, -1.0f, 0.0f}, blue},
    };
    indices = {
        0, 1, 2, 2, 3,0,
        4, 5, 6, 6, 7,4,
        8, 9, 10, 10, 11, 8,
        12,13,14, 14,15,12,
        16,17,18, 18,19,16,
        20,21,22, 22,23,20,
    };

    std::transform(
        vertices.begin(), vertices.end(), vertices.begin(),
        [=](auto v) {
            v.position.x *= size;
            v.position.y *= size;
            v.position.z *= size;
            return v;
        }
    );
}

void TestManager::CreateSphereIndices(std::vector<UINT>& indices, int slices, int stacks) {
    for (int stack = 0; stack < stacks; ++stack) {
        const int sliceMax = slices + 1;
        for (int slice = 0; slice < slices; ++slice) {
            int idx = stack * sliceMax;
            int i0 = idx + (slice + 0) % sliceMax;
            int i1 = idx + (slice + 1) % sliceMax;
            int i2 = i0 + sliceMax;
            int i3 = i1 + sliceMax;

            indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
            indices.push_back(i2); indices.push_back(i1); indices.push_back(i3);
        }
    }
}

void TestManager::GetSphere(std::vector<VertexPN>& vertices, std::vector<UINT>& indices, float radius, int slices, int stacks)
{
    vertices.clear();
    indices.clear();
    CreateSphereVertices(vertices, radius, slices, stacks);
    CreateSphereIndices(indices, slices, stacks);
}
void TestManager::GetSphere(std::vector<VertexPNT>& vertices, std::vector<UINT>& indices, float radius, int slices, int stacks)
{
    vertices.clear();
    indices.clear();
    CreateSphereVertices(vertices, radius, slices, stacks);
    CreateSphereIndices(indices, slices, stacks);
}
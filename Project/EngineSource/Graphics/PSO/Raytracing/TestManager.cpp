#include "TestManager.h"
#include <cassert>
#include "CreateBufferResource.h"
#include "ResourceGarbageCollector.h"
#include "MyMath.h"
#include <algorithm>
using namespace GameEngine;

void TestManager::Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList4* commandList, DXC* dxc, SrvManager* srvManager, RenderPassController* renderPassController,
    TestCamera* testCamera) {
	device_ = device;
	commandList_ = commandList;
	dxc_ = dxc;
    srvManager_ = srvManager;
	renderPassController_ = renderPassController;
    testCamera_ = testCamera;

    // 床とキューブのオブジェクトを生成する.
    CreateSceneObjects();
    // 床とキューブの BLAS を構築する.
    CreateSceneBLAS();
    // シーンに配置して TLAS を構築する.
    CreateSceneTLAS();
    // グローバル Root Signature を用意.
    CreateRootSignatureGlobal();
    // ローカル Root Signature を用意.
    CreateLocalRootSignatureRayGen();
    CreateLocalRootSignatureCHS();
    // コンパイル済みシェーダーよりステートオブジェクトを用意.
    CreateStateObject();
    // レイトレーシング結果格納のためのバッファ(UAV)を用意.
    CreateResultBuffer();
    // 描画で使用する Shader Table を用意.
    CreateShaderTable();
}

void TestManager::Draw() {

    renderPassController_->PrePass("RaytracingPass");

    commandList_->SetComputeRootSignature(rootSignatureGlobal_.Get());
    // TLASのセット
    commandList_->SetComputeRootDescriptorTable(0, srvManager_->GetGPUHandle(tlasSrvIndex_));
    // カメラのセット
    commandList_->SetComputeRootConstantBufferView(1, testCamera_->constBuffer_.GetGpuVirtualAddress());

    //// UAVバッファのセット
    //auto outputDescriptor = srvManager_->GetGPUHandle(renderPassController_->GetUavIndex("RaytracingPass"));
    //commandList_->SetComputeRootDescriptorTable(1, outputDescriptor);

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

    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    GetPlane(vertices, indices);
    meshPlane.vertexBuffer_.Create(vertices);
    meshPlane.indexBuffer_.Create(indices);
    meshPlane.shaderName = DefaultHitgroup_.c_str();

    //========================================================
    // Cube の生成.
    //========================================================
    vertices.clear();
    indices.clear();
    GetColoredCube(vertices, indices);
    meshCube.vertexBuffer_.Create(vertices);
    meshCube.indexBuffer_.Create(indices);
    meshCube.shaderName = DefaultHitgroup_.c_str();
}

void TestManager::CreateSceneBLAS() {
   
    // 床のBLASを作成する
    auto& vp = meshPlane.vertexBuffer_;
    auto& ip = meshPlane.indexBuffer_;
    meshPlane.blas_.Create(commandList_, vp.GetView(), ip.GetView(), vp.GetTotalVertices(), ip.GetTotalIndices());

    // cubeのBLASを作成する
    auto& vc = meshCube.vertexBuffer_;
    auto& ic = meshCube.indexBuffer_;
    meshCube.blas_.Create(commandList_,vc.GetView(),ic.GetView(),vc.GetTotalVertices(),ic.GetTotalIndices());
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
    std::array<CD3DX12_ROOT_PARAMETER, 2> rootParams{};

    // TLAS を t0 レジスタに割り当てて使用する設定.
    CD3DX12_DESCRIPTOR_RANGE descRangeTLAS;
    descRangeTLAS.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    rootParams[0].InitAsDescriptorTable(1, &descRangeTLAS); // t0
    rootParams[1].InitAsConstantBufferView(0); // b0

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = UINT(rootParams.size());
    rootSigDesc.pParameters = rootParams.data();

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

void TestManager::CreateLocalRootSignatureCHS() {
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
        throw std::runtime_error("RootSignature(lrsModel) failed.");
    }

    hr = device_->CreateRootSignature(
        0, blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(rsModel.ReleaseAndGetAddressOf())
    );
    if (FAILED(hr)) {
        throw std::runtime_error("RootSignature(lrsModel) failed.");
    }
    rsModel->SetName(L"lrsModel");
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
    auto blob = dxc_->CompileShader(L"Resources/Shaders/raytracing.hlsl", L"lib_6_5", L"");

    //===================================================================================
    // シェーダーから各関数レコード
    //===================================================================================
    auto lib = rtPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE libdxil = { blob->GetBufferPointer(), blob->GetBufferSize() };
    lib->SetDXILLibrary(&libdxil);

    // 使用する関数名（HLSL側の関数名）をエクスポートする
    lib->DefineExport(L"mainRayGen"); //レイを飛ばす
    lib->DefineExport(L"mainMS"); // レイに当たらなかった場合
    lib->DefineExport(L"mainCHS"); // レイがオブジェクトに当たった時

    //===================================================================================
    // ヒットグループの設定 
    //===================================================================================
    auto hitGroup = rtPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitGroup->SetHitGroupExport(DefaultHitgroup_.c_str()); // C++側(ShaderTable)で指定する名前
    hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
    hitGroup->SetClosestHitShaderImport(L"mainCHS"); // 結びつけるシェーダー名

    //===================================================================================
    // グローバルルートシグネチャの設定
    //===================================================================================
    auto globalRootSignature = rtPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(rootSignatureGlobal_.Get());

    //===================================================================================
    // ローカルルートシグネチャの設定
    //===================================================================================

    // レイが衝突した時
    auto tmprsModel = rtPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    tmprsModel->SetRootSignature(rsModel.Get());
    auto lrsAssocModel = rtPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    lrsAssocModel->AddExport(DefaultHitgroup_.c_str());
    lrsAssocModel->SetSubobjectToAssociate(*tmprsModel);

    // レイの発射
    auto rsRayGen = rtPipeline.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    rsRayGen->SetRootSignature(rsRGS.Get());
    auto lrsAssocRGS = rtPipeline.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    lrsAssocRGS->AddExport(L"mainRayGen");
    lrsAssocRGS->SetSubobjectToAssociate(*rsRayGen);

    //===================================================================================
    // シェーダーの設定
    //===================================================================================
    auto shaderConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    // ペイロード(色情報などレイが運ぶデータ)と、アトリビュート(重心座標など)の最大バイト数
    // ※必要最小限のサイズにしないとパフォーマンスが落ちます
    uint32_t payloadSize = 3 * sizeof(float);   // 例: float3 color
    uint32_t attributeSize = 2 * sizeof(float); // 例: float2 barycentrics
    shaderConfig->Config(payloadSize, attributeSize);

    //=================================================================================== 
    // パイプラインの設定 レイの再帰可能段数
    //===================================================================================
    auto pipelineConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    // 1にすると「カメラからのレイ」のみ。反射（鏡など）を作るなら 2 や 3 にする
    pipelineConfig->Config(1);

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
    //  RayGen : 1
    //  Miss : 1
    //  HitGroup: 2 (オブジェクトは3つ描画するがヒットグループは2つで処理する).
    UINT hitgroupCount = 2;
    UINT raygenSize = 1 * raygenRecordSize;
    UINT missSize = 1 * missRecordSize;
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
        auto id = rtsoProps->GetShaderIdentifier(L"mainMS");
        if (id == nullptr) {
            throw std::logic_error("Not found ShaderIdentifier");
        }
        p += WriteShaderIdentifier(p, id);

        // ローカルルートシグネチャ使用時には他のデータを書き込む.
    }

    // Hit Group 用のシェーダーレコードを書き込み.
    auto hitgroupStart = pStart + raygenRegion + missRegion;
    {
        uint8_t* pRecord = hitgroupStart;

        // plane に対応するシェーダーレコードを書き込む.
        pRecord = WriteShaderRecord(pRecord, meshPlane, hitgroupRecordSize);

        // cube に対応するシェーダーレコードを書き込む.
        pRecord = WriteShaderRecord(pRecord, meshCube, hitgroupRecordSize);
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

void TestManager::GetPlane(std::vector<VertexData>& vertices, std::vector<UINT>& indices) {
    float size = 10.0f;
    Vector4 white = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    VertexData srcVertices[] = {
        VertexData{ {-1.0f, 0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, white },
        VertexData{ {-1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, white },
        VertexData{ { 1.0f, 0.0f,-1.0f }, { 0.0f, 1.0f, 0.0f }, white },
        VertexData{ { 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, white },
    };
    vertices.resize(4);
    std::transform(
        std::begin(srcVertices), std::end(srcVertices), vertices.begin(),
        [=](auto v) {
            v.position.x *= size;
            v.position.z *= size;
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

void TestManager::DeployObjects(std::vector<D3D12_RAYTRACING_INSTANCE_DESC>& instanceDescs) {
    D3D12_RAYTRACING_INSTANCE_DESC templateDesc{};
    templateDesc.InstanceMask = 0xFF;
    templateDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

    // シーンに床、Cubeを2つ配置する
    instanceDescs.resize(3);

    auto& floor = instanceDescs[0];
    auto& cube1 = instanceDescs[1];
    auto& cube2 = instanceDescs[2];

    // 床を配置
    floor = templateDesc;
    Matrix4x4 matrix = Transpose(MakeIdentity4x4());
    std::memcpy(&floor.Transform, &matrix, sizeof(float) * 12);
    floor.InstanceContributionToHitGroupIndex = 0;
    floor.AccelerationStructure = meshPlane.blas_.GetGpuVirtualAddress();

    // Cube(1) 設置.
    cube1 = templateDesc;
    matrix = Transpose(MakeTranslateMatrix(Vector3(- 3.0f, 1.0f, 0.0f)));
    std::memcpy(&cube1.Transform, &matrix, sizeof(float) * 12);
    cube1.InstanceContributionToHitGroupIndex = 1;
    cube1.AccelerationStructure = meshCube.blas_.GetGpuVirtualAddress();

    // Cube(2) 設置.
    cube2 = templateDesc;
    matrix = Transpose(MakeTranslateMatrix(Vector3(+3.0f, 1.0f, 0.0f)));
    std::memcpy(&cube2.Transform, &matrix, sizeof(float) * 12);
    cube2.InstanceContributionToHitGroupIndex = 1;
    cube2.AccelerationStructure = meshCube.blas_.GetGpuVirtualAddress();
}
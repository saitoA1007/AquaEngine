#include "TestRayPipeline.h"
#include <cassert>
using namespace GameEngine;

void TestRayPipeline::Initialize() {

    CreateRootSignatureGlobal();
    CreateStateObject();
    CreateResultBuffer();
    CreateShaderTable();
}

void TestRayPipeline::Draw() {

    commandList_->SetComputeRootSignature(rootSignatureGlobal_.Get());

    // TLASのセット
    //commandList_->SetComputeRootDescriptorTable(0, m_tlasDescriptor.hGpu);
    // UAVバッファのセット
    auto outputDescriptor = renderPassController_->GetSrvHandle("RaytracingPass");
    commandList_->SetComputeRootDescriptorTable(1, outputDescriptor);

    // レイトレーシング結果バッファを UAV 状態へ.
    renderPassController_->PrePass("RaytracingPass");

    // レイトレーシングを開始.
    commandList_->SetPipelineState1(stateObject_.Get());
    commandList_->DispatchRays(&dispatchRayDesc_);

    renderPassController_->PostPass("RaytracingPass");
}

//=============================================
// 
// ルートシグネチャ(Global) を生成します.
// 
//=============================================
void TestRayPipeline::CreateRootSignatureGlobal() {
    std::array<D3D12_ROOT_PARAMETER, 2> rootParams{};

    // TLAS を t0 レジスタに割り当てて使用する設定.
    D3D12_DESCRIPTOR_RANGE descRangeTLAS{};
    descRangeTLAS.BaseShaderRegister = 0;
    descRangeTLAS.NumDescriptors = 1;
    descRangeTLAS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    // 出力バッファ(UAV) を u0 レジスタに割り当てて使用する設定.
    D3D12_DESCRIPTOR_RANGE descRangeOutput{};
    descRangeOutput.BaseShaderRegister = 0;
    descRangeOutput.NumDescriptors = 1;
    descRangeOutput.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

    rootParams[0] = D3D12_ROOT_PARAMETER{};
    rootParams[1] = D3D12_ROOT_PARAMETER{};

    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[0].DescriptorTable.pDescriptorRanges = &descRangeTLAS;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &descRangeOutput;

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

//============================================================================
// 
// レイトレーシング用の StateObject を構築します.
// 
//============================================================================
void TestRayPipeline::CreateStateObject() {
	CD3DX12_STATE_OBJECT_DESC rtPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    //===================================================================================
	// シェーダーファイルの読み込み
    //===================================================================================
    auto blob = dxc_->CompileShader(L"Resources/Shaders/raytracing.hlsl", L"lib_6_8", L"");

    //===================================================================================
	// シェーダーから各関数レコード
    //===================================================================================
    auto lib = rtPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE libdxil = { blob->GetBufferPointer(), blob->GetBufferSize() };
    lib->SetDXILLibrary(&libdxil);

    // 使用する関数名（HLSL側の関数名）をエクスポートする
    lib->DefineExport(L"mainRayGen");
    lib->DefineExport(L"mainMS");
    lib->DefineExport(L"mainCHS");

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
	// シェーダーの設定
    //===================================================================================
    auto shaderConfig = rtPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    // ペイロード(色情報などレイが運ぶデータ)と、アトリビュート(重心座標など)の最大バイト数
    // ※必要最小限のサイズにしないとパフォーマンスが落ちます
    uint32_t payloadSize = 4 * sizeof(float);   // 例: float4 color
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
void TestRayPipeline::CreateResultBuffer() {

    // ここではレイトレーシング用で書き込むためのテクスチャを用意する
    // すでに描画用のテクスチャを作成するクラスが存在しているのでそれを利用する
   
    // 描画するパス
    renderPassController_->AddPass("RaytracingPass", RenderTextureMode::UavOnly);
}

// レイトレーシングで使用する ShaderTable を構築します.
void TestRayPipeline::CreateShaderTable() {
    // 各シェーダーレコードは Shader Identifier を保持する.
    UINT recordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    // グローバルのルートシグネチャ以外の情報を持たないのでレコードサイズはこれだけ.

    // あとはアライメント制約を保つようにする.
    recordSize = RoundUp(recordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

    // シェーダーテーブルのサイズを求める.
    UINT raygenSize = 1 * recordSize; // 今1つの Ray Generation シェーダー.
    UINT missSize = 1 * recordSize;  // 今1つの Miss シェーダー.
    UINT hitGroupSize = 1 * recordSize; // 今1つの HitGroup を使用.

    // 各テーブルの開始位置にアライメント制約があるので調整する.
    auto tableAlign = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    UINT raygenRegion = RoundUp(raygenSize, tableAlign);
    UINT missRegion = RoundUp(missSize, tableAlign);
    UINT hitgroupRegion = RoundUp(hitGroupSize, tableAlign);

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
        memcpy(p, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        p += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

        // ローカルルートシグネチャ使用時には他のデータを書き込む.
    }

    // Miss Shader 用のシェーダーレコードを書き込み.
    auto missStart = pStart + raygenRegion;
    {
        uint8_t* p = missStart;
        auto id = rtsoProps->GetShaderIdentifier(L"mainMS");
        if (id == nullptr) {
            throw std::logic_error("Not found ShaderIdentifier");
        }
        memcpy(p, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        p += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

        // ローカルルートシグネチャ使用時には他のデータを書き込む.
    }

    // Hit Group 用のシェーダーレコードを書き込み.
    auto hitgroupStart = pStart + raygenRegion + missRegion;
    {
        uint8_t* p = hitgroupStart;
        auto id = rtsoProps->GetShaderIdentifier(DefaultHitgroup_.c_str());
        if (id == nullptr) {
            throw std::logic_error("Not found ShaderIdentifier");
        }
        memcpy(p, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        p += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

        // ローカルルートシグネチャ使用時には他のデータを書き込む.
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
    shaderRecordMS.StrideInBytes = recordSize;
    startAddress += missRegion;

    auto& shaderRecordHG = dispatchRayDesc_.HitGroupTable;
    shaderRecordHG.StartAddress = startAddress;
    shaderRecordHG.SizeInBytes = hitGroupSize;
    shaderRecordHG.StrideInBytes = recordSize;
    startAddress += hitgroupRegion;

    dispatchRayDesc_.Width = 1280;
    dispatchRayDesc_.Height = 720;
    dispatchRayDesc_.Depth = 1;
}
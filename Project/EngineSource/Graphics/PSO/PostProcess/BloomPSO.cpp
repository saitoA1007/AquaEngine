#include"BloomPSO.h"
#include"ConvertString.h"
#include"CreateBufferResource.h"
#include <d3dcompiler.h>
#include <cassert>
#include"LogManager.h"
using namespace GameEngine;

void BloomPSO::Initialize(ID3D12Device* device, const std::wstring& vsPath, DXC* dxc,
    const std::wstring brightPsPath, const std::wstring blurPsPath, const std::wstring resultPsPath, const std::wstring compositePsPath) {

    // 初期化を開始するログ
    LogManager::GetInstance().Log("BloomPSO Class start Initialize");

    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // RootSignature: SRV(テクスチャ)のみ
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 1; // 0から始まる
    descriptorRange[0].NumDescriptors = 4; // 数は4つ
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // offsetを自動計算

    D3D12_DESCRIPTOR_RANGE objectDescriptorRange[1] = {};
    objectDescriptorRange[0].BaseShaderRegister = 0; // 0から始まる
    objectDescriptorRange[0].NumDescriptors = 1; // 数は1つ
    objectDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
    objectDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // offsetを自動計算

    // RootParameter作成
    D3D12_ROOT_PARAMETER rootParameters[3] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange; // Tableの中身の配列を指定
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  // CBVを使う
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // PixelShaderで使う
    rootParameters[1].Descriptor.ShaderRegister = 0;  // レジスタ番号0

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
    rootParameters[2].DescriptorTable.pDescriptorRanges = objectDescriptorRange; // Tableの中身の配列を指定
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(objectDescriptorRange); // Tableで利用する数

    descriptionRootSignature.pParameters = rootParameters;  // ルートパラメータ配列へのポインタ
    descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; //0-1の範囲に固定
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMIpmapを使う
    staticSamplers[0].ShaderRegister = 0; // レジスタ番号0を使う
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    // シリアライズしてバイナリにする
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        LogManager::GetInstance().Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    // バイナリを元に生成
    hr = device->CreateRootSignature(0,
        signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // ブレンドモード
    D3D12_BLEND_DESC blendDesc{};
    // すべての色要素を書き込む
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // RasiterzerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc{};
    // 裏面(時計回り)を表示しない
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    // 三角形の中を塗りつぶす
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // シェーダ読み込み
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob;
    vsBlob = dxc->CompileShader(vsPath,
        L"vs_6_0", dxc->dxcUtils_.Get(), dxc->dxcCompiler_.Get(), dxc->includeHandler_.Get());
    assert(vsBlob != nullptr);
    
    Microsoft::WRL::ComPtr<IDxcBlob> brightPsBlob, blurPsBlob, resultPsBlob, blurCompositePsBlob;
    // 明るい部分を抽出する用
    brightPsBlob = dxc->CompileShader(brightPsPath,
        L"ps_6_0", dxc->dxcUtils_.Get(), dxc->dxcCompiler_.Get(), dxc->includeHandler_.Get());
    assert(brightPsBlob != nullptr);
    // ぼかす用
    blurPsBlob = dxc->CompileShader(blurPsPath,
        L"ps_6_0", dxc->dxcUtils_.Get(), dxc->dxcCompiler_.Get(), dxc->includeHandler_.Get());
    assert(blurPsBlob != nullptr);
    // ぼかした結果をまとめる用
    resultPsBlob = dxc->CompileShader(resultPsPath,
        L"ps_6_0", dxc->dxcUtils_.Get(), dxc->dxcCompiler_.Get(), dxc->includeHandler_.Get());
    assert(resultPsBlob != nullptr);
    // 元の画像と合成する用
    blurCompositePsBlob = dxc->CompileShader(compositePsPath,
        L"ps_6_0", dxc->dxcUtils_.Get(), dxc->dxcCompiler_.Get(), dxc->includeHandler_.Get());
    assert(blurCompositePsBlob != nullptr);

    // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    // Depthの機能を無効
    depthStencilDesc.DepthEnable = false;

    // PSO設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
    graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();// RootSignature
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;// InputLayout
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc; // RasterizerState
    graphicsPipelineStateDesc.BlendState = blendDesc;  // ブレンドモードを設定
    graphicsPipelineStateDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };// VertexShader
    graphicsPipelineStateDesc.PS = { brightPsBlob->GetBufferPointer(), brightPsBlob->GetBufferSize() };// PixelShader
    // DepthStencilの設定
    graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
    graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    // 書き込むRTVの情報
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    // 利用するトポロジ（形状）のタイプ。三角形
    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // どのように画面に色を打ち込んむかの設定（気にしなくて良い）
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // 実際に生成
    // 明るい部分を抽出するPipeline
    hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
        IID_PPV_ARGS(&brightPipelineState_));
    assert(SUCCEEDED(hr));

    // ぼかしのPipeline
    graphicsPipelineStateDesc.PS = { blurPsBlob->GetBufferPointer(), blurPsBlob->GetBufferSize() };// PixelShader
    hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
        IID_PPV_ARGS(&blurPipelineState_));
    assert(SUCCEEDED(hr));

    // ぼかしの結果のPipeline
    graphicsPipelineStateDesc.PS = { resultPsBlob->GetBufferPointer(), resultPsBlob->GetBufferSize() };// PixelShader
    hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
        IID_PPV_ARGS(&blurResultPipelineState_));
    assert(SUCCEEDED(hr));

    // 元の画像との合成用のPipeline
    graphicsPipelineStateDesc.PS = { blurCompositePsBlob->GetBufferPointer(), blurCompositePsBlob->GetBufferSize() };// PixelShader
    hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
        IID_PPV_ARGS(&blurCompositePipelineState_));
    assert(SUCCEEDED(hr));

    // 初期化を終了するログ
    LogManager::GetInstance().Log("BloomPSO Class End Initialize\n");

    // Sprite用の頂点リソースを作る
    vertexResourceSprite_ = CreateBufferResource(device, sizeof(VertexData) * 4);

    // リソースの先頭のアドレスから使う
    vertexBufferViewSprite_.BufferLocation = vertexResourceSprite_->GetGPUVirtualAddress();
    // 使用するリソースのサイズは頂点4つ分のサイズ
    vertexBufferViewSprite_.SizeInBytes = sizeof(VertexData) * 4;
    // 1頂点当たりのサイズ
    vertexBufferViewSprite_.StrideInBytes = sizeof(VertexData);
    // 書き込むためのアドレスを取得
    vertexResourceSprite_->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite_));

    // 頂点インデックス
    vertexDataSprite_[0].position = { -1.0f,-1.0f,0.0f,1.0f }; // 左下
    vertexDataSprite_[0].texcoord = { 0.0f,1.0f };
    vertexDataSprite_[1].position = { -1.0f,1.0f,0.0f,1.0f }; // 左上
    vertexDataSprite_[1].texcoord = { 0.0f,0.0f };
    vertexDataSprite_[2].position = { 1.0f,-1.0f,0.0f,1.0f }; // 右下
    vertexDataSprite_[2].texcoord = { 1.0f,1.0f };
    vertexDataSprite_[3].position = { 1.0f, 1.0f,0.0f,1.0f }; // 左上
    vertexDataSprite_[3].texcoord = { 1.0f,0.0f };

    // パラメーター調整用
    bloomParameterResource_ = CreateBufferResource(device, sizeof(ConstBuffer));
    // 書き込むためのアドレスを取得
    bloomParameterResource_->Map(0, nullptr, reinterpret_cast<void**>(&constBuffer_));
    constBuffer_->bloomIteration = 1;
    constBuffer_->highLumMask = 1.0f;
    constBuffer_->sigma = 1.0f;
    constBuffer_->intensity = 0.0f;
}

void BloomPSO::Set(ID3D12GraphicsCommandList* commandList) {
    commandList->SetGraphicsRootSignature(rootSignature_.Get());      
}

void BloomPSO::Draw(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE inputSRV) {
    commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite_);
    commandList->SetGraphicsRootDescriptorTable(0, inputSRV);
    // フルスクリーン三角形
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->DrawInstanced(4, 1, 0, 0);
}
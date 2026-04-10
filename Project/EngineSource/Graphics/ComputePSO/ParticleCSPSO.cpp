#include"ParticleCSPSO.h"
#include"ConvertString.h"
#include"CreateBufferResource.h"
#include <d3dcompiler.h>
#include <cassert>

using namespace GameEngine;

void ParticleCSPSO::Initialize(ID3D12Device* device, DXC* dxc, LogManager* logManager) {

    // 初期化を開始するログ
    if (logManager) {
        logManager->Log("ParticleCSPSO Class start Initialize\n");
    }
    
    // コンピュートの設定
    CreateCS(device, dxc, logManager);

    // VSPSの設定
    CreateVSPS(device, dxc, logManager);

    // 初期化を終了するログ
    if (logManager) {
        logManager->Log("ParticleCSPSO Class End Initialize\n");
    }

}

void ParticleCSPSO::CreateCS(ID3D12Device* device, DXC* dxc, LogManager* logManager) {
    // RootSignature作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // RootSignature
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0; // 0から始まる
    descriptorRange[0].NumDescriptors = 1; // 数は1つ
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; // UAVを使う
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // offsetを自動計算

    // RootParameter作成
    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // すべてで使う
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange; // Tableの中身の配列を指定
    rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数
    descriptionRootSignature.pParameters = rootParameters;  // ルートパラメータ配列へのポインタ
    descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ

    // シリアライズしてバイナリにする
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        logManager->Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    // バイナリを元に生成
    hr = device->CreateRootSignature(0,
        signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&computeRootSignature_));
    assert(SUCCEEDED(hr));

    // シェーダ読み込み
    Microsoft::WRL::ComPtr<IDxcBlob> computeShaderBlob;
    computeShaderBlob = dxc->CompileShader(L"Resources/CS/Particle.CS.hlsl",
        L"cs_6_0", dxc->dxcUtils_.Get(), dxc->dxcCompiler_.Get(), dxc->includeHandler_.Get());
    assert(computeShaderBlob != nullptr);

    // コンピュートパイプラインを設定
    D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc{};
    computePipelineStateDesc.CS = {
        .pShaderBytecode = computeShaderBlob->GetBufferPointer(),
        .BytecodeLength = computeShaderBlob->GetBufferSize(),
    };
    computePipelineStateDesc.pRootSignature = computeRootSignature_.Get();
    hr = device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&computePipelineState_));
}

void ParticleCSPSO::CreateVSPS(ID3D12Device* device, DXC* dxc, LogManager* logManager) {
	// RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// テクスチャデータ用
	D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
	descriptorRangeForInstancing[0].BaseShaderRegister = 0; // 0から始まる
	descriptorRangeForInstancing[0].NumDescriptors = 1; // 数は1つ
	descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // offsetを自動計算

	// パーティクルデータ用
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // 0から始まる
	descriptorRange[0].NumDescriptors = 1; // 数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; // UAVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // offsetを自動計算

	// RootParameter作成。PixelShaderのMaterialとVertexShaderのTransform
	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  // CBVを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0;  // レジスタ番号0
	
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderで使う
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRange; // Tableの中身の配列を指定
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange); // Tableで利用する数

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing; // Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing); // Tableで利用する数
	descriptionRootSignature.pParameters = rootParameters;  // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //0-1の範囲外をリピート 
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // ありったけのMIpmapを使う
	staticSamplers[0].ShaderRegister = 0; // レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う 
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob, errorBlob;
	// シリアライズしてバイナリにする
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		logManager->Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に生成
	hr = device->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// RasiterzerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob, pixelShaderBlob;

	// Shaderをコンパイルする
	vertexShaderBlob = dxc->CompileShader(L"Resources/Shaders/CS/Particle.VS.hlsl",
		L"vs_6_0", dxc->dxcUtils_.Get(), dxc->dxcCompiler_.Get(), dxc->includeHandler_.Get());
	assert(vertexShaderBlob != nullptr);

	pixelShaderBlob = dxc->CompileShader(L"Resources/Shaders/CS/Particle.PS.hlsl",
		L"ps_6_0", dxc->dxcUtils_.Get(), dxc->dxcCompiler_.Get(), dxc->includeHandler_.Get());
	assert(pixelShaderBlob != nullptr);

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 書き込みします
	//depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	// 比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// ブレンドモードの設定
	D3D12_BLEND_DESC blendDesc_;
	blendDesc_.RenderTarget[0].BlendEnable = TRUE; // ブレンドを有効化
	blendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // アルファ値のソース
	blendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファ値の加算ブレンド
	blendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO; // アルファ値のデスティネーション
	blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // (1-SrcA)

	// PSO設定
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();// RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;// InputLayout
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc; // RasterizerState
	graphicsPipelineStateDesc.BlendState = blendDesc_;// ブレンドモードを設定
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };// VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };// PixelShader
	// DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// 利用するトポロジ（形状）のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込んむかの設定（気にしなくて良い）
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 実際に生成
	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}
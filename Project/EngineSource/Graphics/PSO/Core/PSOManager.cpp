#include"PSOManager.h"
#include "LogManager.h"
#include <cassert>

using namespace GameEngine;

void PSOManager::Initialize(ID3D12Device* device, DXC* dxc) {
	LogManager::GetInstance().Log("Initialize PSOManager Start");
	device_ = device;
	dxc_ = dxc;
	// ラスタライザの全パターン生成
	rasterizerBuiler_.Initialize();
	// ブレンドモードの全パターン生成
	blendBuilder_.Initialize();
	// シェーダーコンパイラの初期唖k
	shaderCompiler_.Initialize(dxc);
	LogManager::GetInstance().Log("Initialize PSOManager End");
}

void PSOManager::RegisterPSO(const std::string& name, const CreatePSOData& psoData) {

	// 既に登録されていたら飛ばす
	if (psoList_.find(name) != psoList_.end()) {
		return;
	}

	CreatePSO(name,psoData);

	LogManager::GetInstance().Log("PSO registerd name : " + name);
}

void PSOManager::RegisterPSO(const std::string& name, const CreatePSOData& psoData, RootSignatureBuilder* rootSignature, InputLayoutBuilder* inputLayout) {

    // 既に登録されていたら飛ばす
    if (psoList_.find(name) != psoList_.end()) {
        return;
    }

    // シェーダーをコンパイル
    LogManager::GetInstance().Log("Compiling vertex shader");
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::VS, psoData.vsPath);

    LogManager::GetInstance().Log("Compiling pixel shader");
    Microsoft::WRL::ComPtr<IDxcBlob> psBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::PS, psoData.psPath);

    if (!vsBlob || !psBlob) {
        LogManager::GetInstance().Log("Shader compilation failed for: " + name);
        return;
    }

    // ルートシグネチャが登録されていなければ生成する
    if (rootSignatureList_.find(psoData.rootSigName) == rootSignatureList_.end()) {
        RootSignatureData rootSignatureData;
        rootSignatureData.rootSignature = rootSignature->GetRootSignature();
        rootSignatureData.parameterTypes = rootSignature->GetParameterTypes();
        // RootSignatureを保存
        rootSignatureList_[psoData.rootSigName] = rootSignatureData;
    }    

    // DepthStencilStateの設定
     // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    if (psoData.isDepthEnable) {
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = psoData.depthMask;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    } else {
        depthStencilDesc.DepthEnable = false;
    }

    // PSO設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignatureList_[psoData.rootSigName].rootSignature.Get();
    psoDesc.InputLayout = inputLayout->GetInputLayoutDesc();
    psoDesc.RasterizerState = rasterizerBuiler_.GetRasterizerDesc(psoData.drawMode);
    psoDesc.BlendState = blendBuilder_.GetBlendDesc(psoData.blendMode);
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.PrimitiveTopologyType = psoData.primitiveType;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // PSOの生成
    PSOData pso;
    // リンクするルートシグネチャを保存
    pso.rootSigName = psoData.rootSigName;

    // 実際に生成
    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso.graphicsPipelineState));
    assert(SUCCEEDED(hr));

    // PSOを保存
    psoList_[name] = pso;

    LogManager::GetInstance().Log("PSO registerd name : " + name);
}

void PSOManager::RegisterShadowMapPSO(const std::string& name, const CreatePSOData& psoData, RootSignatureBuilder* rootSignature, InputLayoutBuilder* inputLayout) {
    // 既に登録されていたら飛ばす
    if (psoList_.find(name) != psoList_.end()) {
        return;
    }

    // シェーダーをコンパイル
    LogManager::GetInstance().Log("Compiling vertex shader");
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::VS, psoData.vsPath);

    if (!vsBlob) {
        LogManager::GetInstance().Log("Shader compilation failed for: " + name);
        return;
    }

    // ルートシグネチャが登録されていなければ生成する
    if (rootSignatureList_.find(psoData.rootSigName) == rootSignatureList_.end()) {
        RootSignatureData rootSignatureData;
        rootSignatureData.rootSignature = rootSignature->GetRootSignature();
        rootSignatureData.parameterTypes = rootSignature->GetParameterTypes();
        // RootSignatureを保存
        rootSignatureList_[psoData.rootSigName] = rootSignatureData;
    }

    // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = psoData.depthMask;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // かリングを設定
    D3D12_RASTERIZER_DESC rast{};
    rast.FillMode = D3D12_FILL_MODE_SOLID;
    rast.CullMode = D3D12_CULL_MODE_BACK;
    // シャドウアクネ対策
    rast.DepthBias = 1000;
    rast.SlopeScaledDepthBias = 1.0f;
    rast.DepthBiasClamp = 0.0f;

    // PSO設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignatureList_[psoData.rootSigName].rootSignature.Get();
    psoDesc.InputLayout = inputLayout->GetInputLayoutDesc();
    psoDesc.RasterizerState = rast;
    psoDesc.BlendState = blendBuilder_.GetBlendDesc(psoData.blendMode);
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { nullptr,0 };
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.NumRenderTargets = 0;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    psoDesc.PrimitiveTopologyType = psoData.primitiveType;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // PSOの生成
    PSOData pso;
    // リンクするルートシグネチャを保存
    pso.rootSigName = psoData.rootSigName;

    // 実際に生成
    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso.graphicsPipelineState));
    assert(SUCCEEDED(hr));

    // PSOを保存
    psoList_[name] = pso;

    LogManager::GetInstance().Log("PSO registerd name : " + name);
}

void PSOManager::LoadFromJson(const std::string& fileName) {

	// 読み込みJSONファイルのフルパスを合成する
	std::string filePath = kDirectoryPath + fileName + ".json";
	// 読み込み用ファイルストリーム
	std::ifstream ifs;
	// ファイルを読み込み用に開く
	ifs.open(filePath);

	// ファイルオープン失敗
	if (ifs.fail()) {
		std::string message = "Failed open data file for load.";
		MessageBoxA(nullptr, message.c_str(), "PSOManager", 0);
		assert(0);
	}

    json root;

    // json文字列からjsonのデータ構造に展開
    ifs >> root;
    // ファイルを閉じる
    ifs.close();

	LogManager::GetInstance().Log("PSO registerd name : " + fileName);
}

void PSOManager::CreatePSO(const std::string& psoName, const CreatePSOData& psoData) {
	LogManager::GetInstance().Log("PSO create start : " + psoName);

    // シェーダーをコンパイル
    LogManager::GetInstance().Log("Compiling vertex shader");
    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::VS, psoData.vsPath);

    LogManager::GetInstance().Log("Compiling pixel shader");
    Microsoft::WRL::ComPtr<IDxcBlob> psBlob = shaderCompiler_.CompileShader(ShaderCompiler::Type::PS, psoData.psPath);

    if (!vsBlob || !psBlob) {
        LogManager::GetInstance().Log("Shader compilation failed for: " + psoName);
        return;
    }

    // ルートシグネチャが登録されていなければ生成する
    // RootSignatureの生成
    RootSignatureBuilder rootSigBuilder;
    if (rootSignatureList_.find(psoData.rootSigName) == rootSignatureList_.end()) {
        rootSigBuilder.Initialize(device_);
        rootSigBuilder.CreateRootSignatureFromReflection(dxc_->GetIDxcUtils(), vsBlob.Get(), psBlob.Get());
        RootSignatureData rootSignatureData;
        rootSignatureData.rootSignature = rootSigBuilder.GetRootSignature();
        rootSignatureData.parameterTypes = rootSigBuilder.GetParameterTypes();
        // RootSignatureを保存
        rootSignatureList_[psoData.rootSigName] = rootSignatureData;
    }

    // InputLayoutの生成
    InputLayoutBuilder inputLayoutBuilder;
    inputLayoutBuilder.CreateInputLayoutFromReflection(dxc_->GetIDxcUtils(), vsBlob.Get());
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = inputLayoutBuilder.GetInputLayoutDesc();

    // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    if (psoData.isDepthEnable) {
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    } else {
        depthStencilDesc.DepthEnable = false;
    }

    // PSOの設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    // InputLayout
    psoDesc.InputLayout = inputLayoutDesc;
    // RootSignature
    psoDesc.pRootSignature = rootSignatureList_[psoData.rootSigName].rootSignature.Get();
    // シェーダー
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    // BlendState
    psoDesc.BlendState = blendBuilder_.GetBlendDesc(psoData.blendMode);
    // RasterizerState
    psoDesc.RasterizerState = rasterizerBuiler_.GetRasterizerDesc(psoData.drawMode);
    // DepthStencilState
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    // 描画タイプ
    psoDesc.PrimitiveTopologyType = psoData.primitiveType;
    
    // PSOの生成
    PSOData pso;
    // リンクするルートシグネチャを保存
    pso.rootSigName = psoData.rootSigName;

    // 実際に生成
    HRESULT hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso.graphicsPipelineState));
    assert(SUCCEEDED(hr));

    // PSOを保存
    psoList_[psoName] = pso;

    LogManager::GetInstance().Log("PSO create end : " + psoName);
}

ID3D12RootSignature* PSOManager::GetRootSignature(const std::string& name) {
	auto it = rootSignatureList_.find(name);
	if (it != rootSignatureList_.end()) {
		return it->second.rootSignature.Get();
	}

	LogManager::GetInstance().Log("RootSignature not found: " + name);
	assert(0);
	return nullptr;
}

ID3D12PipelineState* PSOManager::GetPSO(const std::string& name) {
	auto it = psoList_.find(name);
	if (it != psoList_.end()) {
		return it->second.graphicsPipelineState.Get();
	}

	LogManager::GetInstance().Log("PSO not found: " + name);
	assert(0);
	return nullptr;
}

DrawPsoData PSOManager::GetDrawPsoData(const std::string& PsoName) const {

    auto pso = psoList_.find(PsoName);
    if (pso == psoList_.end()) {
        assert(0);
    }

    auto root = rootSignatureList_.find(pso->second.rootSigName);
    if (root == rootSignatureList_.end()) {
        assert(0);
    }

    DrawPsoData drawData;
    drawData.rootSignature = root->second.rootSignature.Get();
    drawData.graphicsPipelineState = pso->second.graphicsPipelineState.Get();
    return drawData;
}

void PSOManager::DefaultLoadPSO() {

    LogManager::GetInstance().Log("Loading default PSOs");

    // デフォルトの3Dオブジェクト用PSO
    CreatePSOData default3D;
    default3D.rootSigName = "Default3D";
    default3D.vsPath = L"Resources/Shaders/Object3d.VS.hlsl";
    default3D.psPath = L"Resources/Shaders/Object3d.PS.hlsl";
    default3D.drawMode = DrawModel::FillFront;
    default3D.blendMode = BlendMode::kBlendModeNormal;
    default3D.isDepthEnable = true;
    RootSignatureBuilder rootSigBuilder;
    rootSigBuilder.Initialize(device_);
    rootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL);
    rootSigBuilder.AddCBVParameter(2, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddSRVDescriptorTable(1, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 1, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddSRVDescriptorTable(2, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount) + static_cast<uint32_t>(SrvHeapTypeCount::SystemMaxCount),2, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_COMPARISON_FUNC_LESS_EQUAL);
    rootSigBuilder.CreateRootSignature();
    InputLayoutBuilder inputLayoutBuilder;
    inputLayoutBuilder.CreateDefaultObjElement();
    RegisterPSO("Default3D", default3D, &rootSigBuilder, &inputLayoutBuilder);

    // 加算合成用PSO
    default3D.blendMode = BlendMode::kBlendModeAdd;
    RegisterPSO("Additive3D", default3D, &rootSigBuilder, &inputLayoutBuilder);

    // デフォルトのスプライト用PSO
    CreatePSOData defaultSprite;
    defaultSprite.rootSigName = "Default2D";
    defaultSprite.vsPath = L"Resources/Shaders/Sprite.VS.hlsl";
    defaultSprite.psPath = L"Resources/Shaders/Sprite.PS.hlsl";
    defaultSprite.drawMode = DrawModel::None;
    defaultSprite.blendMode = BlendMode::kBlendModeNormal;
    defaultSprite.isDepthEnable = false;
    RootSignatureBuilder spriteRoot;
    spriteRoot.Initialize(device_);
    spriteRoot.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_ALL);
    spriteRoot.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
    spriteRoot.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    spriteRoot.CreateRootSignature();
    InputLayoutBuilder spriteInput;
    spriteInput.CreateDefaultSpriteElement();
    RegisterPSO("DefaultSprite", defaultSprite,&spriteRoot,&spriteInput);
    defaultSprite.blendMode = BlendMode::kBlendModeAdd;
    RegisterPSO("AdditiveSprite", defaultSprite, &spriteRoot,&spriteInput);

    // インスタンシング描画用PSO
    CreatePSOData instancing3D;
    instancing3D.rootSigName = "Instancing3D";
    instancing3D.vsPath = L"Resources/Shaders/Particle.VS.hlsl";
    instancing3D.psPath = L"Resources/Shaders/Particle.PS.hlsl";
    instancing3D.drawMode = DrawModel::FillFront;
    instancing3D.blendMode = BlendMode::kBlendModeNormal;
    instancing3D.isDepthEnable = true;
    RootSignatureBuilder instancingRootSigBuilder;
    instancingRootSigBuilder.Initialize(device_);
    instancingRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    instancingRootSigBuilder.AddSRVDescriptorTable(0, 1,0, D3D12_SHADER_VISIBILITY_VERTEX);
    instancingRootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount),0, D3D12_SHADER_VISIBILITY_PIXEL);
    instancingRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    instancingRootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    instancingRootSigBuilder.CreateRootSignature();
    RegisterPSO("Instancing3D", instancing3D, &instancingRootSigBuilder, &inputLayoutBuilder);

    // インスタンシング描画の加算合成用PSO
    instancing3D.blendMode = BlendMode::kBlendModeAdd;
    RegisterPSO("AdditiveInstancing3D", instancing3D, &instancingRootSigBuilder, &inputLayoutBuilder);

    // グリッド描画用のPSO
    CreatePSOData grid;
    grid.rootSigName = "Grid";
    grid.vsPath = L"Resources/Shaders/Grid.VS.hlsl";
    grid.psPath = L"Resources/Shaders/Grid.PS.hlsl";
    grid.drawMode = DrawModel::None;
    grid.blendMode = BlendMode::kBlendModeNormal;
    grid.isDepthEnable = true;
    RootSignatureBuilder gridRootSigBuilder;
    gridRootSigBuilder.Initialize(device_);
    gridRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    gridRootSigBuilder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL);
    gridRootSigBuilder.CreateRootSignature();
    InputLayoutBuilder gridInputLayoutBuilder;
    gridInputLayoutBuilder.CreateDefaultObjElement();
    RegisterPSO("Grid", grid, &gridRootSigBuilder, &gridInputLayoutBuilder);

    // デバックライン描画用のPSO
    CreatePSOData line;
    line.rootSigName = "Line";
    line.vsPath = L"Resources/Shaders/Primitive.VS.hlsl";
    line.psPath = L"Resources/Shaders/Primitive.PS.hlsl";
    line.drawMode = DrawModel::None;
    line.blendMode = BlendMode::kBlendModeNormal;
    line.isDepthEnable = true;
    line.primitiveType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    RootSignatureBuilder lineRootSigBuilder;
    lineRootSigBuilder.Initialize(device_);
    lineRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    lineRootSigBuilder.CreateRootSignature();
    InputLayoutBuilder lineInputLayoutBuilder;
    lineInputLayoutBuilder.CreateDefaultLineElement();
    RegisterPSO("Line", line, &lineRootSigBuilder, &lineInputLayoutBuilder);

    // アニメーション描画用のPSO
    CreatePSOData animation;
    animation.rootSigName = "Animation";
    animation.vsPath = L"Resources/Shaders/SkinningObject3d.VS.hlsl";
    animation.psPath = L"Resources/Shaders/Object3d.PS.hlsl";
    animation.drawMode = DrawModel::FillFront;
    animation.blendMode = BlendMode::kBlendModeNormal;
    animation.isDepthEnable = true;
    RootSignatureBuilder animationRootSigBuilder;
    animationRootSigBuilder.Initialize(device_);
    animationRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    animationRootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddSRVDescriptorTable(0, 1,0, D3D12_SHADER_VISIBILITY_VERTEX);
    animationRootSigBuilder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_ALL);
    animationRootSigBuilder.AddCBVParameter(2, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddSRVDescriptorTable(1, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 1, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddSRVDescriptorTable(2, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount) + static_cast<uint32_t>(SrvHeapTypeCount::SystemMaxCount), 2, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    animationRootSigBuilder.AddSampler(1, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_COMPARISON_FUNC_LESS_EQUAL);
    animationRootSigBuilder.CreateRootSignature();
    InputLayoutBuilder animationInputLayoutBuilder;
    animationInputLayoutBuilder.CreateDefaultAnimationElement();
    RegisterPSO("Animation", animation, &animationRootSigBuilder, &animationInputLayoutBuilder);

    // skyboxのpso設定
    CreatePSOData skybox;
    skybox.rootSigName = "Skybox";
    skybox.vsPath = L"Resources/Shaders/Skybox.VS.hlsl";
    skybox.psPath = L"Resources/Shaders/Skybox.PS.hlsl";
    skybox.drawMode = DrawModel::FillFront;
    skybox.blendMode = BlendMode::kBlendModeNormal;
    skybox.isDepthEnable = true;
    skybox.depthMask = D3D12_DEPTH_WRITE_MASK::D3D12_DEPTH_WRITE_MASK_ZERO;
    RootSignatureBuilder skyRoot;
    skyRoot.Initialize(device_);
    skyRoot.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    skyRoot.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    skyRoot.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
    skyRoot.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_VERTEX);
    skyRoot.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    skyRoot.CreateRootSignature();
    InputLayoutBuilder skyInput;
    skyInput.CreateDefaultObjElement();
    RegisterPSO("Skybox", skybox, &skyRoot, &skyInput);

    // ShadowMap用のPSO設定
    CreatePSOData shadowMap;
    shadowMap.rootSigName = "ShadowMap";
    shadowMap.vsPath = L"Resources/Shaders/ShadowMap.VS.hlsl";
    shadowMap.blendMode = BlendMode::kBlendModeNormal;
    RootSignatureBuilder shadowMapRootSigBuilder;
    shadowMapRootSigBuilder.Initialize(device_);
    shadowMapRootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_VERTEX);
    shadowMapRootSigBuilder.AddCBVParameter(1, D3D12_SHADER_VISIBILITY_VERTEX);
    shadowMapRootSigBuilder.CreateRootSignature();
    RegisterShadowMapPSO("ShadowMap", shadowMap, &shadowMapRootSigBuilder, &inputLayoutBuilder);

    LogManager::GetInstance().Log("Default PSOs loaded");
}

void PSOManager::DeaultLoadPostEffectPSO() {
    // 画像のコピーを描画
    //CreatePSOData copy;
    //copy.rootSigName = "DefaultPostEffect";
    //copy.vsPath = L"Resources/Shaders/PostEffect/FullScreen.VS.hlsl";
    //copy.psPath = L"Resources/Shaders/PostEffect/Copy.PS.hlsl";
    //copy.drawMode = DrawModel::FillFront;
    //copy.blendMode = BlendMode::kBlendModeNone;
    //copy.isDepthEnable = false;
    //RootSignatureBuilder copyRootSigBuilder;
    //copyRootSigBuilder.Initialize(device_);
    //copyRootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount) + static_cast<uint32_t>(SrvHeapTypeCount::SystemMaxCount), 0, D3D12_SHADER_VISIBILITY_PIXEL);
    //copyRootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    //copyRootSigBuilder.CreateRootSignature();
    InputLayoutBuilder inputLayoutBuilder;
    inputLayoutBuilder.CreateNone();
    //RegisterPSO("Copy", copy, &copyRootSigBuilder, &inputLayoutBuilder);

    // ヴィネットを作成
    CreatePSOData defaultPostEffect;
    defaultPostEffect.rootSigName = "DefaultPostEffect";
    defaultPostEffect.vsPath = L"Resources/Shaders/PostEffect/FullScreen.VS.hlsl";
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/Vignetting/Vignetting.PS.hlsl";
    defaultPostEffect.drawMode = DrawModel::FillFront;
    defaultPostEffect.blendMode = BlendMode::kBlendModeNone;
    defaultPostEffect.isDepthEnable = false;
    RootSignatureBuilder rootSigBuilder;
    rootSigBuilder.Initialize(device_);
    rootSigBuilder.AddSRVDescriptorTable(0, static_cast<uint32_t>(SrvHeapTypeCount::TextureMaxCount) + static_cast<uint32_t>(SrvHeapTypeCount::SystemMaxCount),
        0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddCBVParameter(0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.AddSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSigBuilder.CreateRootSignature();
    RegisterPSO("Vignetting", defaultPostEffect, &rootSigBuilder, &inputLayoutBuilder);

    // スキャンラインを作成
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/ScanLine/ScanLine.PS.hlsl";
    RegisterPSO("ScanLine", defaultPostEffect, &rootSigBuilder, &inputLayoutBuilder);

    // ラジアルブラーを作成
    defaultPostEffect.psPath = L"Resources/Shaders/PostEffect/RadialBlur/RadialBlur.PS.hlsl";
    RegisterPSO("RadialBlur", defaultPostEffect, &rootSigBuilder, &inputLayoutBuilder);
}
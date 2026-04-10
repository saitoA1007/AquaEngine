#include"RootSignatureBuilder.h"
#include <cassert>
#include"LogManager.h"
#include <d3d12shader.h>

using namespace GameEngine;

void RootSignatureBuilder::Initialize(ID3D12Device* device) {
	device_ = device;
    // SRVを作成する配列のメモリをあらかじめ確保しておく
    descriptorRanges_.reserve(8); 
}

void RootSignatureBuilder::AddCBVParameter(uint32_t shaderRegister, D3D12_SHADER_VISIBILITY visibility) {
	D3D12_ROOT_PARAMETER param = {};
	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	param.ShaderVisibility = visibility;
	param.Descriptor.ShaderRegister = shaderRegister;
	rootParameters_.push_back(param);
}

void RootSignatureBuilder::AddSRVDescriptorTable(uint32_t shaderRegister, uint32_t arryNum, uint32_t spaceNum, D3D12_SHADER_VISIBILITY visibility) {

    D3D12_DESCRIPTOR_RANGE range{};
    range.BaseShaderRegister = shaderRegister; // 始まる番号
    range.RegisterSpace = spaceNum; // 使用するスペース
    range.NumDescriptors = arryNum; // 数
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // offsetを自動計算
    descriptorRanges_.push_back(std::move(range));

	D3D12_ROOT_PARAMETER param{};
	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	param.ShaderVisibility = visibility; // 使うシェーダー
	param.DescriptorTable.pDescriptorRanges = &descriptorRanges_.back(); // Tableの中身の配列を指定
	param.DescriptorTable.NumDescriptorRanges = 1; // Tableで利用する数
	rootParameters_.push_back(param);
}

void RootSignatureBuilder::AddSampler(uint32_t shaderRegister, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE texAddress, D3D12_SHADER_VISIBILITY visibility, D3D12_COMPARISON_FUNC func) {
    D3D12_STATIC_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = filter; // バイリニアフィルタ
    samplerDesc.AddressU = texAddress;
    samplerDesc.AddressV = texAddress;
    samplerDesc.AddressW = texAddress;
    samplerDesc.ComparisonFunc = func;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplerDesc.ShaderRegister = shaderRegister; // レジスタ番号
    samplerDesc.ShaderVisibility = visibility; // シェーダーモード
    staticSamplers_.push_back(samplerDesc);
}

void RootSignatureBuilder::CreateRootSignature() {
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob, errorBlob;

    // ルートシグネチャの作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.NumParameters = static_cast<UINT>(rootParameters_.size());
    descriptionRootSignature.pParameters = rootParameters_.data();
    descriptionRootSignature.NumStaticSamplers = static_cast<UINT>(staticSamplers_.size());
    descriptionRootSignature.pStaticSamplers = staticSamplers_.data();
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
   
	// シリアライズしてバイナリにする
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		LogManager::GetInstance().Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}
	// バイナリを元に生成
	hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}

void RootSignatureBuilder::CreateRootSignatureFromReflection(IDxcUtils* utils,IDxcBlob* vsBlob, IDxcBlob* psBlob) {

	DxcBuffer reflectionBuffer{};
	reflectionBuffer.Encoding = DXC_CP_ACP;

	// 頂点シェーダーとピクセルシェーダーのリフレクションを取得
	IDxcBlob* shaderBlobs[2] = { vsBlob, psBlob };
	D3D12_SHADER_VISIBILITY visibilities[2] = {
		D3D12_SHADER_VISIBILITY_VERTEX,
		D3D12_SHADER_VISIBILITY_PIXEL
	};

    // 同じパラメータを統合するためのマップ
    std::map<uint32_t, ResourceInfo> cbvMap;
    std::map<uint32_t, ResourceInfo> srvMap;
    std::map<uint32_t, ResourceInfo> samplerMap;

    // 分析する
    for (uint32_t index = 0; index < 2; ++index) {
        ReflectionBoundResourceToMap(utils, reflectionBuffer, shaderBlobs[index], visibilities[index], cbvMap, srvMap, samplerMap);
    }

    // 取得したパラメータマップからルートパラメータを作成
    CreateParametersFromMaps(cbvMap, srvMap, samplerMap);

    // ルートシグネチャの作成
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = static_cast<UINT>(rootParameters_.size());
    rootSigDesc.pParameters = rootParameters_.data();
    rootSigDesc.NumStaticSamplers = static_cast<UINT>(staticSamplers_.size());
    rootSigDesc.pStaticSamplers = staticSamplers_.data();
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // シリアライズしてバイナリにする
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            LogManager::GetInstance().Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        }
        assert(false);
    }

    // バイナリを元に生成
    hr = device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}

void RootSignatureBuilder::ReflectionBoundResource(IDxcUtils* utils,DxcBuffer reflectionBuffer, IDxcBlob* shaderBlob, D3D12_SHADER_VISIBILITY visibility) {
    reflectionBuffer.Ptr = shaderBlob->GetBufferPointer();
    reflectionBuffer.Size = shaderBlob->GetBufferSize();

    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
    HRESULT hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection));
    if (FAILED(hr)) {
        assert(0);
    }

    D3D12_SHADER_DESC shaderDesc{};
    reflection->GetDesc(&shaderDesc);

    // バインドされたリソースを解析
    for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
        reflection->GetResourceBindingDesc(i, &bindDesc);

        D3D12_ROOT_PARAMETER param{};
        param.ShaderVisibility = visibility;

        switch (bindDesc.Type) {

            // 定数バッファ
        case D3D_SIT_CBUFFER: {
            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            param.Descriptor.ShaderRegister = bindDesc.BindPoint;
            param.Descriptor.RegisterSpace = bindDesc.Space;
            rootParameters_.push_back(param);
            parameterTypes_.push_back(ParameterType::CBV);

            std::string s = "none";
            if (param.ShaderVisibility == D3D12_SHADER_VISIBILITY_VERTEX) {
                s = "Vertex";
            } else if (param.ShaderVisibility == D3D12_SHADER_VISIBILITY_PIXEL) {
                s = "Pixel";
            } else if (param.ShaderVisibility == D3D12_SHADER_VISIBILITY_ALL) {
                s = "All";
            }

            LogManager::GetInstance().Log("arrayNum" + std::to_string(rootParameters_.size() - 1) + " / Type : CBV / registerNum : " + std::to_string(bindDesc.BindPoint) + " / visibility : " + s);
            break;
        }


            // SRV
        case D3D_SIT_TEXTURE: {
            D3D12_DESCRIPTOR_RANGE range{};
            range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            range.NumDescriptors = bindDesc.BindCount;
            range.BaseShaderRegister = bindDesc.BindPoint;
            range.RegisterSpace = bindDesc.Space;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            descriptorRanges_.push_back(std::move(range));

            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param.DescriptorTable.NumDescriptorRanges = 1;
            param.DescriptorTable.pDescriptorRanges = &descriptorRanges_.back();
            rootParameters_.push_back(param);
            parameterTypes_.push_back(ParameterType::SRV);
            std::string s = "none";
            if (param.ShaderVisibility == D3D12_SHADER_VISIBILITY_VERTEX) {
                s = "Vertex";
            } else if (param.ShaderVisibility == D3D12_SHADER_VISIBILITY_PIXEL) {
                s = "Pixel";
            } else if (param.ShaderVisibility == D3D12_SHADER_VISIBILITY_ALL) {
                s = "All";
            }
            LogManager::GetInstance().Log("arrayNum" + std::to_string(rootParameters_.size() - 1) + " / Type : SRV / registerNum : " + std::to_string(bindDesc.BindPoint) + " / visibility : " + s);
            break;
        }


        // サンプラー
        case D3D_SIT_SAMPLER: {
            D3D12_STATIC_SAMPLER_DESC samplerDesc{};
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            samplerDesc.ShaderRegister = bindDesc.BindPoint;
            samplerDesc.RegisterSpace = bindDesc.Space;
            samplerDesc.ShaderVisibility = visibility;
            staticSamplers_.push_back(samplerDesc);
            break;
        }
        }
    }
}

void RootSignatureBuilder::ReflectionBoundResourceToMap(IDxcUtils* utils, DxcBuffer reflectionBuffer, IDxcBlob* shaderBlob, D3D12_SHADER_VISIBILITY visibility,
    std::map<uint32_t, ResourceInfo>& cbvMap, std::map<uint32_t, ResourceInfo>& srvMap, std::map<uint32_t, ResourceInfo>& samplerMap) {

    reflectionBuffer.Ptr = shaderBlob->GetBufferPointer();
    reflectionBuffer.Size = shaderBlob->GetBufferSize();

    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
    HRESULT hr = utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection));
    if (FAILED(hr)) {
        assert(0);
    }

    D3D12_SHADER_DESC shaderDesc{};
    reflection->GetDesc(&shaderDesc);

    // 解析したデータをマップに保存する
    for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
        reflection->GetResourceBindingDesc(i, &bindDesc);

        switch (bindDesc.Type) {
        case D3D_SIT_CBUFFER: {
            auto it = cbvMap.find(bindDesc.BindPoint);
            if (it != cbvMap.end()) {
                // 同じものがあれば統合
                it->second.visibility = MergeVisibility(it->second.visibility, visibility);
            } else {
                // 存在しない場合、新しくcbvを作成
                ResourceInfo info;
                info.visibility = visibility;
                info.bindPoint = bindDesc.BindPoint;
                info.space = bindDesc.Space;
                info.bindCount = 1;
                cbvMap[bindDesc.BindPoint] = info;
            }
            break;
        }

        case D3D_SIT_TEXTURE: {
            auto it = srvMap.find(bindDesc.BindPoint);
            if (it != srvMap.end()) {
                // 同じものがあれば統合
                it->second.visibility = MergeVisibility(it->second.visibility, visibility);
            } else {
                // 存在しない場合、新しくsrvを作成
                ResourceInfo info;
                info.visibility = visibility;
                info.bindPoint = bindDesc.BindPoint;
                info.space = bindDesc.Space;
                info.bindCount = bindDesc.BindCount;
                srvMap[bindDesc.BindPoint] = info;
            }
            break;
        }

        case D3D_SIT_SAMPLER: {
            auto it = samplerMap.find(bindDesc.BindPoint);
            if (it != samplerMap.end()) {
                // 同じものがあれば統合
                it->second.visibility = MergeVisibility(it->second.visibility, visibility);
            } else {
                // 存在しない場合、新しくsamplerを作成
                ResourceInfo info;
                info.visibility = visibility;
                info.bindPoint = bindDesc.BindPoint;
                info.space = bindDesc.Space;
                info.bindCount = 1;
                samplerMap[bindDesc.BindPoint] = info;
            }
            break;
        }
        }
    }
}

void RootSignatureBuilder::CreateParametersFromMaps(const std::map<uint32_t, ResourceInfo>& cbvMap, const std::map<uint32_t, ResourceInfo>& srvMap, const std::map<uint32_t, ResourceInfo>& samplerMap){
    // CBVを追加
    for (const auto& pair : cbvMap) {
        const ResourceInfo& info = pair.second;

        D3D12_ROOT_PARAMETER param{};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        param.ShaderVisibility = info.visibility;
        param.Descriptor.ShaderRegister = info.bindPoint;
        param.Descriptor.RegisterSpace = info.space;
        rootParameters_.push_back(param);
        parameterTypes_.push_back(ParameterType::CBV);

        std::string s = "none";
        if (info.visibility == D3D12_SHADER_VISIBILITY_VERTEX) {
            s = "Vertex";
        } else if (info.visibility == D3D12_SHADER_VISIBILITY_PIXEL) {
            s = "Pixel";
        } else if (info.visibility == D3D12_SHADER_VISIBILITY_ALL) {
            s = "All";
        }

        LogManager::GetInstance().Log("arrayNum" + std::to_string(rootParameters_.size() - 1) + " / Type : CBV / registerNum : " + std::to_string(info.bindPoint) + " / visibility : " + s);
    }

    // SRVを追加
    for (const auto& pair : srvMap) {
        const ResourceInfo& info = pair.second;

        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = info.bindCount;
        range.BaseShaderRegister = info.bindPoint;
        range.RegisterSpace = info.space;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        descriptorRanges_.push_back(std::move(range));

        D3D12_ROOT_PARAMETER param{};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility = info.visibility;
        param.DescriptorTable.NumDescriptorRanges = 1;
        param.DescriptorTable.pDescriptorRanges = &descriptorRanges_.back();
        rootParameters_.push_back(param);
        parameterTypes_.push_back(ParameterType::SRV);

        std::string s = "none";
        if (info.visibility == D3D12_SHADER_VISIBILITY_VERTEX) {
            s = "Vertex";
        } else if (info.visibility == D3D12_SHADER_VISIBILITY_PIXEL) {
            s = "Pixel";
        } else if (info.visibility == D3D12_SHADER_VISIBILITY_ALL) {
            s = "All";
        }

        LogManager::GetInstance().Log("arrayNum" + std::to_string(rootParameters_.size() - 1) + " / Type : SRV / registerNum : " + std::to_string(info.bindPoint) + " / visibility : " + s);
    }

    // サンプラーを追加
    for (const auto& pair : samplerMap) {
        const ResourceInfo& info = pair.second;

        D3D12_STATIC_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.ShaderRegister = info.bindPoint;
        samplerDesc.RegisterSpace = info.space;
        samplerDesc.ShaderVisibility = info.visibility;
        staticSamplers_.push_back(samplerDesc);
    }
}

D3D12_SHADER_VISIBILITY RootSignatureBuilder::MergeVisibility(D3D12_SHADER_VISIBILITY v1, D3D12_SHADER_VISIBILITY v2) {
    // 既にAllならそのまま返す
    if (v1 == D3D12_SHADER_VISIBILITY_ALL || v2 == D3D12_SHADER_VISIBILITY_ALL) {
        return D3D12_SHADER_VISIBILITY_ALL;
    }

    // 異なる形で使用されている場合はAll
    if (v1 != v2) {
        return D3D12_SHADER_VISIBILITY_ALL;
    }

    // 同じならそのまま返す
    return v1;
}